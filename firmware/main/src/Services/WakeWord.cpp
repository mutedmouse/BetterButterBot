#include "WakeWord.h"
#include <algorithm>
#include <cstring>
#include <new>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/idf_additions.h>
#include <tensorflow/lite/micro/micro_allocator.h>
#include <tensorflow/lite/micro/micro_resource_variable.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Util/ServiceLocator.h>
#include "frontend_util.h"
#include "Log/Log.h"

DEFINE_LOG(WakeWord)

namespace {
	uint32_t nowMs(){
		return (uint32_t) (esp_timer_get_time() / 1000);
	}

	// Arena sizes to try; microWakeWord models need ~23-26 KB but esp-nn kernels vary.
	constexpr size_t ArenaSizes[] = { 32 * 1024, 48 * 1024, 64 * 1024 };
	constexpr size_t VarArenaSize = 1024;
}

WakeWord::WakeWord(){
	window = std::clamp<size_t>(CONFIG_BB_WAKEWORD_WINDOW, 1, MaxWindow);
	cutoff = (uint8_t) std::clamp(CONFIG_BB_WAKEWORD_CUTOFF, 1, 255);

	// Ops used by microWakeWord streaming MixedNet models.
	resolver.AddCallOnce();
	resolver.AddVarHandle();
	resolver.AddReshape();
	resolver.AddReadVariable();
	resolver.AddStridedSlice();
	resolver.AddConcatenation();
	resolver.AddAssignVariable();
	resolver.AddConv2D();
	resolver.AddMul();
	resolver.AddAdd();
	resolver.AddMean();
	resolver.AddFullyConnected();
	resolver.AddLogistic();
	resolver.AddQuantize();
	resolver.AddDepthwiseConv2D();
	resolver.AddAveragePool2D();
	resolver.AddMaxPool2D();
	resolver.AddPad();
	resolver.AddPack();
	resolver.AddSplitV();

	if(!loadModel()){
		return;
	}

	// Feature frontend: must match the training pipeline (30 ms window, 10 ms step, 40 channels).
	// Populated here rather than in the task because it uses floats.
	FrontendConfig cfg;
	FrontendFillConfigWithDefaults(&cfg);
	cfg.window.size_ms = 30;
	cfg.window.step_size_ms = 10;
	cfg.filterbank.num_channels = Features;
	cfg.filterbank.lower_band_limit = 125.0f;
	cfg.filterbank.upper_band_limit = 7500.0f;
	cfg.noise_reduction.smoothing_bits = 10;
	cfg.noise_reduction.even_smoothing = 0.025f;
	cfg.noise_reduction.odd_smoothing = 0.06f;
	cfg.noise_reduction.min_signal_remaining = 0.05f;
	cfg.pcan_gain_control.enable_pcan = 1;
	cfg.pcan_gain_control.strength = 0.95f;
	cfg.pcan_gain_control.offset = 80.0f;
	cfg.pcan_gain_control.gain_bits = 21;
	cfg.log_scale.enable_log = 1;
	cfg.log_scale.scale_shift = 6;

	frontend = static_cast<FrontendState*>(heap_caps_calloc(1, sizeof(FrontendState), MALLOC_CAP_SPIRAM));
	micBuf = static_cast<int16_t*>(heap_caps_malloc(SamplesPer10ms * 2 * sizeof(int16_t), MALLOC_CAP_SPIRAM));
	if(frontend == nullptr || micBuf == nullptr || !FrontendPopulateState(&cfg, frontend, 16000)){
		CMF_LOG(WakeWord, LogLevel::Error, "Frontend init failed");
		return;
	}

	lockoutUntil = nowMs() + 3000; // let the frontend's noise estimate settle

	// Stack in PSRAM: this task never touches flash/NVS itself.
	if(xTaskCreatePinnedToCoreWithCaps(&WakeWord::taskEntry, "WakeWord", 4096, this, 3, &task, 1, MALLOC_CAP_SPIRAM) != pdPASS){
		CMF_LOG(WakeWord, LogLevel::Error, "Task create failed");
		task = nullptr;
		return;
	}
	CMF_LOG(WakeWord, LogLevel::Info, "Listening for wake word (cutoff %u/255, window %u, stride %d, arena %u)",
			cutoff, (unsigned) window, stride, (unsigned) arenaSize);
}

WakeWord::~WakeWord(){
	stopping = true;
	while(task != nullptr && !parked){
		vTaskDelay(pdMS_TO_TICKS(20));
	}
	if(interpreter){
		interpreter->~MicroInterpreter();
	}
	heap_caps_free(interpreterMem);
	heap_caps_free(arena);
	heap_caps_free(varArena);
	if(frontend){
		FrontendFreeStateContents(frontend);
		heap_caps_free(frontend);
	}
	heap_caps_free(micBuf);
	if(mapHandle){
		esp_partition_munmap(mapHandle);
	}
}

bool WakeWord::loadModel(){
	partition = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, "wakeword");
	if(partition == nullptr){
		CMF_LOG(WakeWord, LogLevel::Warning, "No wakeword partition; wake word disabled");
		return false;
	}

	if(esp_partition_mmap(partition, 0, partition->size, ESP_PARTITION_MMAP_DATA, (const void**) &modelData, &mapHandle) != ESP_OK){
		CMF_LOG(WakeWord, LogLevel::Error, "Failed to map wakeword partition");
		modelData = nullptr;
		return false;
	}

	// A TFLite flatbuffer carries the "TFL3" identifier at offset 4; an erased partition is 0xFF.
	if(memcmp(modelData + 4, "TFL3", 4) != 0){
		CMF_LOG(WakeWord, LogLevel::Warning, "No model in wakeword partition; wake word disabled");
		return false;
	}

	const tflite::Model* model = tflite::GetModel(modelData);
	if(model->version() != TFLITE_SCHEMA_VERSION){
		CMF_LOG(WakeWord, LogLevel::Error, "Model schema %lu != %d", model->version(), TFLITE_SCHEMA_VERSION);
		return false;
	}

	varArena = static_cast<uint8_t*>(heap_caps_aligned_alloc(16, VarArenaSize, MALLOC_CAP_SPIRAM));
	interpreterMem = static_cast<uint8_t*>(heap_caps_aligned_alloc(16, sizeof(tflite::MicroInterpreter), MALLOC_CAP_SPIRAM));
	if(varArena == nullptr || interpreterMem == nullptr){
		return false;
	}

	for(size_t size : ArenaSizes){
		arena = static_cast<uint8_t*>(heap_caps_aligned_alloc(16, size, MALLOC_CAP_SPIRAM));
		if(arena == nullptr){
			return false;
		}

		// Streaming state lives in resource variables with their own small arena.
		tflite::MicroAllocator* varAllocator = tflite::MicroAllocator::Create(varArena, VarArenaSize);
		tflite::MicroResourceVariables* vars = tflite::MicroResourceVariables::Create(varAllocator, 20);
		interpreter = new(interpreterMem) tflite::MicroInterpreter(model, resolver, arena, size, vars);

		if(interpreter->AllocateTensors() == kTfLiteOk){
			arenaSize = size;
			break;
		}

		interpreter->~MicroInterpreter();
		interpreter = nullptr;
		heap_caps_free(arena);
		arena = nullptr;
	}

	if(interpreter == nullptr){
		CMF_LOG(WakeWord, LogLevel::Error, "Tensor allocation failed");
		return false;
	}

	const TfLiteTensor* in = interpreter->input(0);
	const TfLiteTensor* out = interpreter->output(0);
	if(in->dims->size != 3 || in->dims->data[2] != Features || in->type != kTfLiteInt8
	   || out->type != kTfLiteUInt8 || out->dims->data[out->dims->size - 1] != 1){
		CMF_LOG(WakeWord, LogLevel::Error, "Unexpected model I/O shape");
		interpreter->~MicroInterpreter();
		interpreter = nullptr;
		return false;
	}
	stride = in->dims->data[1];
	CMF_LOG(WakeWord, LogLevel::Info, "Model loaded, arena used %u of %u bytes",
			(unsigned) interpreter->arena_used_bytes(), (unsigned) arenaSize);
	return true;
}

void WakeWord::pause(){
	if(pauseDepth.fetch_add(1) > 0 || task == nullptr){
		return;
	}
	// Wait for the task to finish its current 10 ms read and park, so the caller owns the mic.
	for(int i = 0; i < 20 && !parked; ++i){
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void WakeWord::resume(){
	if(pauseDepth.load() > 0 && pauseDepth.fetch_sub(1) == 1){
		// States often hand back to idle while their last sentence is still starting to play
		// (e.g. "Got it" after "never mind"), before Audio reports playing.
		lockoutUntil = nowMs() + SelfAudioLockoutMs;
	}
}

void WakeWord::beginExternal(){
	if(interpreter == nullptr || pauseDepth.load() == 0 || !parked){
		return;
	}
	resetDetector();
	FrontendReset(frontend);
	lockoutUntil = nowMs() + SelfAudioLockoutMs;
}

void WakeWord::feedExternal(const int16_t* mono, size_t count){
	// Only while our own task is parked, so the frontend/interpreter have a single user.
	if(interpreter == nullptr || pauseDepth.load() == 0 || !parked){
		return;
	}
	Audio* audio = Application::getApp()->getService<Audio>();
	if(audio != nullptr && audio->isPlaying()){
		resetDetector();
		FrontendReset(frontend);
		lockoutUntil = nowMs() + SelfAudioLockoutMs;
		return;
	}
	process(mono, count);
}

void WakeWord::resetDetector(){
	probs.fill(0);
	strideStep = 0;
}

void WakeWord::taskEntry(void* arg){
	static_cast<WakeWord*>(arg)->loop();
	vTaskDeleteWithCaps(nullptr);
}

void WakeWord::loop(){
	while(!stopping){
		if(pauseDepth.load() > 0){
			if(!parked){
				parked = true;
				resetDetector();
				FrontendReset(frontend);
			}
			vTaskDelay(pdMS_TO_TICKS(20));
			continue;
		}
		parked = false;

		const int32_t samples = ServiceLocator::MicInputInstance->getStereo({ micBuf, SamplesPer10ms * 2 });
		if(samples <= 0){
			vTaskDelay(pdMS_TO_TICKS(10));
			continue;
		}
		const size_t frames = (size_t) samples / 2;
		for(size_t i = 0; i < frames; ++i){
			micBuf[i] = (int16_t) (((int32_t) micBuf[2 * i] + micBuf[2 * i + 1]) / 2);
		}

		// Don't wake on our own voice or sound effects. The model was trained on synthetic voices, so
		// the robot's TTS ("Got it", "OK") can score as high as a real "butterbot": drop everything
		// heard while audio plays, forget the frontend's state, and keep ignoring detections until
		// the speaker/mic buffers have drained the tail of our own speech.
		Audio* audio = Application::getApp()->getService<Audio>();
		if(audio != nullptr && audio->isPlaying()){
			resetDetector();
			FrontendReset(frontend);
			lockoutUntil = nowMs() + SelfAudioLockoutMs;
			continue;
		}

		process(micBuf, frames);

	}
	parked = true;
}

void WakeWord::process(const int16_t* mono, size_t count){
	while(count > 0){
		size_t used = 0;
		const FrontendOutput feat = FrontendProcessSamples(frontend, mono, count, &used);
		mono += used;
		count -= used;
		if(used == 0){
			break;
		}
		if(feat.size != (size_t) Features){
			continue;
		}

		// uint16 log-mel (~0..670) -> int8, the same quantization used in training.
		int8_t* dst = interpreter->input(0)->data.int8 + strideStep * Features;
		for(int i = 0; i < Features; ++i){
			const int32_t v = ((int32_t) feat.values[i] * 256 + 333) / 666 - 128;
			dst[i] = (int8_t) std::clamp<int32_t>(v, -128, 127);
		}
		if(++strideStep < stride){
			continue;
		}
		strideStep = 0;

		if(interpreter->Invoke() != kTfLiteOk){
			continue;
		}

		const uint8_t p = interpreter->output(0)->data.uint8[0];
		probIdx = (probIdx + 1) % window;
		probs[probIdx] = p;

		if(nowMs() < lockoutUntil){
			continue;
		}

		uint32_t sum = 0;
		for(size_t i = 0; i < window; ++i){
			sum += probs[i];
		}
		if(sum > (uint32_t) cutoff * window){
			CMF_LOG(WakeWord, LogLevel::Info, "Wake word detected (avg %lu/255)", (unsigned long) (sum / window));
			resetDetector();
			lockoutUntil = nowMs() + LockoutMs;
			OnWake.broadcast();
		}
	}
}
