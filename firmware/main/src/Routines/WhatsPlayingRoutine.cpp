#include "WhatsPlayingRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Phone/Phone.h"
#include "Phone/MediaInfo.h"
#include <Util/ServiceLocator.h>

DEFINE_LOG(WhatsPlayingRoutine)

Routine::TickingState WhatsPlayingRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();
    Phone* phone = app->getService<Phone>();

    if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !phone){
        CMF_LOG(WhatsPlayingRoutine, LogLevel::Error, "Missing required service(s)");
        return TickingState::Done;
    }

    auto playText = [&](const std::string& text) {
        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text);
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
    };

    if(!phone->isConnected()){
        const int16_t id = Phrases::get(Phrase::PhoneNotConnected);
        if(id >= 0){
            PhoneNotConnectedData bbData{};
            bbData.id = static_cast<uint8_t>(id);
            com->sendData(BB::State::Scenario, BB::Action::Scenario::PhoneNotConnected, bbData);
            playText(Phrases::map(Phrase::PhoneNotConnected, id));
        }
        return TickingState::Done;
    }

    const MediaState mediaState = phone->getMediaState();
    const MediaInfo& mediaInfo = phone->getMedia();

    WhatsPlayingData bbData{};
    bbData.title = mediaInfo.title;
    bbData.artist = mediaInfo.artist;
    bbData.album = mediaInfo.album;

    if(mediaState == MediaState::Stopped){
        const int16_t id = Phrases::get(Phrase::PhoneNotPlaying);
        if(id < 0){
	        return TickingState::Done;
        }

        bbData.id = static_cast<uint8_t>(id);
        com->sendData(BB::State::Scenario, BB::Action::Scenario::PhoneWhatsPlaying, bbData);
        playText(Phrases::map(Phrase::PhoneNotPlaying, id));
    }else{
        const int16_t id = Phrases::get(Phrase::PhonePlaying);
        if(id < 0) return TickingState::Done;

        bbData.id = static_cast<uint8_t>(id);
        com->sendData(BB::State::Scenario, BB::Action::Scenario::PhoneWhatsPlaying, bbData);
        playText(mediaInfo.title + " by " + mediaInfo.artist);
    }

    return TickingState::Done;
}
