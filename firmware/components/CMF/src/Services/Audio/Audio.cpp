#include "Audio.h"
#include <utility>

DEFINE_LOG(Audio)

Audio::Audio(StrongObjectPtr<I2S> i2s, OutputPin enablePin, bool internalStack) : Audio(std::move(i2s), internalStack){
	this->enablePin = enablePin;
}

Audio::Audio(StrongObjectPtr<I2S> i2s, bool internalStack) :
		Super(CONFIG_CMF_AUDIO_TICK_INTERVAL / portTICK_PERIOD_MS, CONFIG_CMF_AUDIO_STACK_SIZE, CONFIG_CMF_AUDIO_THREAD_PRIORITY, CONFIG_CMF_AUDIO_CPU_CORE, internalStack), i2s(std::move(i2s)){
	tickSemaphore = xSemaphoreCreateBinary();
	xSemaphoreTake(tickSemaphore, 0);

	playSemaphore = xSemaphoreCreateBinary();
	xSemaphoreGive(playSemaphore);

	if(enablePin){
		enablePin->driver->write(enablePin->port, true);
	}
}

Audio::~Audio(){
	stop();

	if(enablePin){
		enablePin->driver->write(enablePin->port, false);
	}
}

void Audio::play(AudioGenerator* generator, std::unique_ptr<AudioSource> source){
	if(!enabled) return;

	if(playing){
		internalStop();
	}else{
		OnAudioStatusChanged.broadcast(true);
	}

	if(enablePin){
		enablePin->driver->write(enablePin->port, true);
	}

	generator->open(std::move(source));
	this->currentGenerator = generator;

	playing = true;
	if(xSemaphoreTake(playSemaphore, 0) == pdFALSE){
		CMF_LOG(Audio, LogLevel::Warning, "play semaphore shouldn't be taken!");
	}

	xSemaphoreGive(tickSemaphore);
}

void Audio::play(std::span<GenSourcePair> sources){
	if(sources.empty()){
		CMF_LOG(Audio, LogLevel::Warning, "Received empty sources span");
		return;
	}
	if(sources.size() == 1){
		play(sources[0].first, std::move(sources[0].second));
		return;
	}

	if(!enabled) return;

	if(playing){
		internalStop();
	}else{
		OnAudioStatusChanged.broadcast(true);
	}

	if(enablePin){
		enablePin->driver->write(enablePin->port, true);
	}

	auto pair = std::move(sources[0]);


	this->currentGenerator = pair.first;
	currentGenerator->open(std::move(pair.second));

	playing = true;
	if(xSemaphoreTake(playSemaphore, 0) == pdFALSE){
		CMF_LOG(Audio, LogLevel::Warning, "play semaphore shouldn't be taken!");
	}

	for(int i = 1; i < sources.size(); i++){
		sourceQueue.push(std::move(sources[i]));
	}

	xSemaphoreGive(tickSemaphore);
}

void Audio::enqueue(AudioGenerator* generator, std::unique_ptr<AudioSource> source){
	if(!playing){
		play(generator, std::move(source));
		return;
	}

	if(xSemaphoreTake(tickSemaphore, portMAX_DELAY) != pdTRUE){
		CMF_LOG(Audio, LogLevel::Warning, "Couldn't take lock before queueing!");
		return;
	}

	//Also check after lock was taken just in case we locked after the final tick()
	if(!playing){
		xSemaphoreGive(tickSemaphore);
		play(generator, std::move(source));
		return;
	}

	sourceQueue.emplace(generator, std::move(source));

	xSemaphoreGive(tickSemaphore);
}

void Audio::enqueue(std::span<std::pair<AudioGenerator*, std::unique_ptr<AudioSource>>> sources){
	if(sources.empty()){
		CMF_LOG(Audio, LogLevel::Warning, "Received empty sources span");
		return;
	}
	if(sources.size() == 1){
		enqueue(sources[0].first, std::move(sources[0].second));
		return;
	}

	if(!playing){
		play(sources);
		return;
	}

	if(xSemaphoreTake(tickSemaphore, portMAX_DELAY) != pdTRUE){
		CMF_LOG(Audio, LogLevel::Warning, "Couldn't take lock before queueing!");
		return;
	}

	//Also check after tick lock was taken just in case we locked after the final tick
	if(!playing){
		xSemaphoreGive(tickSemaphore);
		play(sources);
		return;
	}

	for(int i = 0; i < sources.size(); i++){
		sourceQueue.push(std::move(sources[i]));
	}

	xSemaphoreGive(tickSemaphore);
}

void Audio::stop(){
	if(playing){
		OnAudioStatusChanged.broadcast(false);
	}
	internalStop();
}

void Audio::tick(float deltaTime) noexcept{
	Super::tick(deltaTime);

	if(!xSemaphoreTake(tickSemaphore, portMAX_DELAY)){
		CMF_LOG(Audio, LogLevel::Warning, "tick: failed to take tickSemaphore");
		return;
	}

	if(!playing){
		return;
	}

	const size_t bytesToTransfer = currentGenerator->getData((uint8_t*)dataBuf.data(), BufSize * sizeof(int16_t));

	CMF_LOG(Audio, LogLevel::Debug, "audio tick bytes: %d", bytesToTransfer);

	if(bytesToTransfer == 0){
		if(sourceQueue.empty()){
			CMF_LOG(Audio, LogLevel::Info, "SourceQueue empty, stopping");
			xSemaphoreGive(tickSemaphore);
			stop();
			return;
		}

		CMF_LOG(Audio, LogLevel::Info, "Popping next source/gen from queue");

		auto pair = std::move(sourceQueue.front());
		sourceQueue.pop();

		if(currentGenerator != pair.first){
			currentGenerator->close();
		}

		this->currentGenerator = pair.first;
		currentGenerator->open(std::move(pair.second));
	}

	if(gain != 1.0f){
		for(auto& sample : dataBuf){
			sample = (int16_t)(sample * gain);
		}
	}

	i2s->write(reinterpret_cast<uint8_t*>(dataBuf.data()), bytesToTransfer);

	xSemaphoreGive(tickSemaphore);
}

void Audio::internalStop(){
	if(!enabled || !playing){
		return;
	}

	if(!xSemaphoreTake(tickSemaphore, portMAX_DELAY)){
		CMF_LOG(Audio, LogLevel::Warning, "internalStop: failed to take tickSemaphore");
		return;
	}

	if(enablePin){
		enablePin->driver->write(enablePin->port, false);
	}
	currentGenerator->close();
	while(!sourceQueue.empty()){
		sourceQueue.pop();
	}
	playing = false;
	xSemaphoreGive(playSemaphore);
}

void Audio::setEnabled(bool enabled){
	Audio::enabled = enabled;
	if(enabled) return;

	stop();
}

bool Audio::isEnabled() const{
	return enabled;
}

void Audio::setGain(float gain){
	this->gain = gain;
}

bool Audio::isPlaying() const{
	return playing;
}

bool Audio::waitEnd(TickType_t ticks){
	if(xSemaphoreTake(playSemaphore, ticks) != pdTRUE) return false;

	xSemaphoreGive(playSemaphore);
	return true;
}
