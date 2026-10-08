#include "CurrentTimeRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <cstdio>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/Time.h"

DEFINE_LOG(CurrentTimeRoutine)

Routine::TickingState CurrentTimeRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();
    Time* timeService = app->getService<Time>();

    if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr || timeService == nullptr){
        CMF_LOG(CurrentTimeRoutine, LogLevel::Error, "Missing required service(s)");
        abort();
    }

    if(!timeService->isConfigured()){
        const int16_t id = Phrases::get(Phrase::CurrentTimeNotConfigured);
        if(id < 0){
            CMF_LOG(CurrentTimeRoutine, LogLevel::Warning, "Phrases::get returned no phrase id for CurrentTimeNotConfigured");
            return TickingState::Done;
        }

        CurrentTimeData data{};
        data.status = CurrentTimeData::Status::NoTime;
        data.currentTime = {};
        com->sendData(BB::State::Scenario, BB::Action::Scenario::CurrentTime, data);

        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(Phrase::CurrentTimeNotConfigured, id));
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
        return TickingState::Done;
    }

    const tm now = timeService->getTime();
    const int hour24 = now.tm_hour;
    const int minute = now.tm_min;
    const char* ampm = hour24 >= 12 ? "PM" : "AM";
    int hour12 = hour24 % 12;
    if(hour12 == 0) hour12 = 12;

    char timeBuf[16];
    snprintf(timeBuf, sizeof(timeBuf), "%d:%02d %s", hour12, minute, ampm);

    const int16_t id = Phrases::get(Phrase::CurrentTimeShow);
    if(id < 0){
        CMF_LOG(CurrentTimeRoutine, LogLevel::Warning, "Phrases::get returned no phrase id for CurrentTimeShow");
        return TickingState::Done;
    }

    const std::string templ = Phrases::map(Phrase::CurrentTimeShow, id);

    char buf[256];
    snprintf(buf, sizeof(buf), templ.c_str(), timeBuf);

    CurrentTimeData data{};
    data.status = CurrentTimeData::Status::ShowTime;
    data.currentTime = now;
    com->sendData(BB::State::Scenario, BB::Action::Scenario::CurrentTime, data);

    auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, std::string(buf));
    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
    audio->waitEnd(portMAX_DELAY);

    return TickingState::Done;
}
