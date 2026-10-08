#include "ObjDet.h"
#include "Devices/Camera.h"
#include "CameraStream.h"
#include "Util/stdafx.h"
#include <Core/Application.h>

DEFINE_LOG(ObjDet)

const std::unordered_map<ObjClass, const char*> ObjDet::ClassNames = {
		{ ObjClass::Backpack, "Backpack" },
		{ ObjClass::Bottle, "Bottle" },
		{ ObjClass::Controller, "Controller" },
		{ ObjClass::Keyboard, "Keyboard" },
		{ ObjClass::Lamp, "Lamp" },
		{ ObjClass::Laptop, "Laptop" },
		{ ObjClass::Mug, "Mug" },
		{ ObjClass::Notebook, "Notebook" },
		{ ObjClass::Phone, "Phone" },
		{ ObjClass::Plant, "Plant" }
};

ObjDet::ObjDet(){
	// MicroMutableOpResolver has a fixed slot count and AddX() rejects duplicates,
	// so set up the op set once at construction rather than on every load.
	resolver.AddQuantize();
	resolver.AddSub();
	resolver.AddMul();
	resolver.AddDiv();
	resolver.AddAdd();
	resolver.AddRelu();
	resolver.AddSoftmax();
	resolver.AddConv2D();
	resolver.AddFullyConnected();
	resolver.AddPad();
	resolver.AddMean();
	resolver.AddReshape();
	resolver.AddAveragePool2D();
	resolver.AddDepthwiseConv2D();

	cam = Application::getApp()->getDevice<Camera>();
}

ObjDet::~ObjDet(){
	deinit();
	unloadModel();
}

void ObjDet::loadModel(){
	// Hold the camera from load to unload (see FaceDet::loadModel): a running stream pauses here.
	CameraStream::claim(CameraStream::ObjectDetection);

	if(interpreter){
		CMF_LOG(ObjDet, Error, "Model already loaded!");
		return;
	}

	if(inited){
		CMF_LOG(ObjDet, Error, "Cannot load model while ObjDet is inited!");
		return;
	}

	uint32_t startMillis = millis();

	const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, "objdet");
	if(part == nullptr){
		CMF_LOG(ObjDet, Error, "Can't find model partition");
		return;
	}

	auto err = esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA, (const void**) &modelData, &dataHndl);
	if(err != ESP_OK){
		CMF_LOG(ObjDet, Error, "Failed mapping memory");
		modelData = nullptr;
		dataHndl = 0;
		return;
	}

	model = tflite::GetModel(modelData);
	if(model->version() != TFLITE_SCHEMA_VERSION){
		CMF_LOG(ObjDet, Error, "Model provided is schema version %lu not equal to supported version %d.", model->version(), TFLITE_SCHEMA_VERSION);
		esp_partition_munmap(dataHndl);
		dataHndl = 0;
		modelData = nullptr;
		model = nullptr;
		return;
	}

	tensorPool.assign(PoolSize, 0);

	interpreter = std::make_unique<tflite::MicroInterpreter>(model, resolver, tensorPool.data(), PoolSize);

	TfLiteStatus status = interpreter->AllocateTensors();
	if(status != kTfLiteOk){
		CMF_LOG(ObjDet, Error, "Error allocating tensors");
		interpreter.reset();
		std::vector<uint8_t>().swap(tensorPool);
		esp_partition_munmap(dataHndl);
		dataHndl = 0;
		modelData = nullptr;
		model = nullptr;
		return;
	}

	CMF_LOG(ObjDet, Debug, "load time: %d", (int)millis() - (int)startMillis);
}

void ObjDet::unloadModel(){
	if(!interpreter){
		CameraStream::release(CameraStream::ObjectDetection);   // also after a failed load
		return;
	}

	if(inited){
		CMF_LOG(ObjDet, Error, "Cannot unload model while ObjDet is inited!");
		return;
	}

	uint32_t startMillis = millis();

	interpreter.reset();
	std::vector<uint8_t>().swap(tensorPool);

	if(dataHndl){
		esp_partition_munmap(dataHndl);
		dataHndl = 0;
	}
	modelData = nullptr;
	model = nullptr;
	CameraStream::release(CameraStream::ObjectDetection);   // the camera stream may resume

	CMF_LOG(ObjDet, Debug, "unload time: %d", (int)millis() - (int)startMillis);
}

void ObjDet::init(){
	if(inited) return;

	if(!interpreter){
		CMF_LOG(ObjDet, Error, "Model not loaded!");
		return;
	}

	cam->setRes(FRAMESIZE_128X128);
	cam->setFormat(PIXFORMAT_RGB565);
	if(cam->init() != ESP_OK){
		CMF_LOG(ObjDet, Error, "Failed initing camera");
		return;
	}

	inited = true;
}

void ObjDet::deinit(){
	if(!inited) return;
	// cam->deinit();
	inited = false;
}

std::vector<ObjClass> ObjDet::detect(){
	if(!interpreter){
		CMF_LOG(ObjDet, Error, "Detect request, but model not loaded");
		return {};
	}

	if(!inited){
		CMF_LOG(ObjDet, Error, "Detect request, but not inited");
		return {};
	}

	/*std::unordered_map<ObjClass, int> hits;
	for(int i = 0; i < 3; i++){
		const auto result = once();

		for(int j = 0; j < 2; j++){
			if(!hits.contains(result[j].cls)){
				hits.insert(std::make_pair(result[j].cls, 0));
			}
		}

		if(result[0].score >= 0.9){
			hits[result[0].cls] += 3;
		}else if(result[1].score >= 0.4){
			hits[result[0].cls] += 2;
			hits[result[1].cls] += 1;
		}
	}

	std::vector<Result> results;
	for(const auto& pair : hits){
		results.emplace_back(pair.first, pair.second);
	}
	std::sort(results.begin(), results.end(), [](const Result& a, const Result& b){ return a.score > b.score; });
	results.erase(std::remove_if(results.begin(), results.end(), [](const Result& r){ return r.score <= 6; }), results.end());*/

	auto results = once();
	/*for(const auto& res : results){
		printf("%s: %.3f\n", ClassNames.at(res.cls), res.score);
	}*/

	if(results.size() < 2) return {};

	std::vector<ObjClass> out;
	if(results[0].score >= 0.9){
		out.push_back(results[0].cls);
	}else if(results[1].score >= 0.4){
		out.push_back(results[0].cls);
		out.push_back(results[1].cls);
	}

	return out;
}

std::vector<ObjDet::Result> ObjDet::once(){
	TfLiteTensor* input = interpreter->input(0);
	TfLiteTensor* output = interpreter->output(0);

	if(!getFrame(input->data.data)) return {};

	auto t = millis();
	const auto status = interpreter->Invoke();
	if(status != kTfLiteOk){
		CMF_LOG(ObjDet, Error, "Inference failed");
		return {};
	}
	CMF_LOG(ObjDet, Info, "Inference done in %llu ms", millis() - t);

	std::vector<Result> results;
	results.reserve(static_cast<size_t>(ObjClass::COUNT));
	for(int i = 0; i < static_cast<size_t>(ObjClass::COUNT); i++){
		const float score = (float) output->data.uint8[i] / 255.0f;
		results.emplace_back(Result { (ObjClass) i, score });
	}
	std::sort(results.begin(), results.end(), [](const Result& a, const Result& b){ return a.score > b.score; });

	return results;
}

bool ObjDet::getFrame(void* dst){
	auto frame = cam->getFrame();
	if(frame == nullptr){
		CMF_LOG(ObjDet, Error, "Failed fetching frame");
		return false;
	}

	const int srcWidth = 128;
	const int srcHeight = 128;
	const int dstWidth = 120;
	const int dstHeight = 120;
	const int srcChannels = 2; // RGB565
	const int dstChannels = 3; // RGB888

	uint8_t* srcBuffer = frame->buf;
	uint8_t* dstBuffer = (uint8_t*) dst;

	int startX = (srcWidth - dstWidth) / 2;
	int startY = (srcHeight - dstHeight) / 2;

	for (int y = 0; y < dstHeight; ++y) {
		for (int x = 0; x < dstWidth; ++x) {
			int srcIndex = ((startY + y) * srcWidth + (startX + x)) * srcChannels;
			int dstIndex = (y * dstWidth + x) * dstChannels;

			uint16_t pixel = (srcBuffer[srcIndex] << 8) | srcBuffer[srcIndex + 1];
			uint8_t r = (pixel >> 11) & 0x1F;
			uint8_t g = (pixel >> 5) & 0x3F;
			uint8_t b = pixel & 0x1F;

			dstBuffer[dstIndex] = (r << 3); // Convert 5-bit red to 8-bit
			dstBuffer[dstIndex + 1] = (g << 2); // Convert 6-bit green to 8-bit
			dstBuffer[dstIndex + 2] = (b << 3); // Convert 5-bit blue to 8-bit
		}
	}

	cam->releaseFrame();

	return true;
}
