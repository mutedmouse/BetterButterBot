#include "WhatsThisRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <cstdio>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/ObjDet.h"
#include <Util/ServiceLocator.h>

DEFINE_LOG(WhatsThisRoutine)

Routine::TickingState WhatsThisRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();

    if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr || !ServiceLocator::ObjDetInstance){
        CMF_LOG(WhatsThisRoutine, LogLevel::Error, "Missing required service(s)");
        abort();
    }

    ServiceLocator::ObjDetInstance->loadModel();

    ServiceLocator::ObjDetInstance->init();
    const std::vector<ObjClass> detections = ServiceLocator::ObjDetInstance->detect();
    ServiceLocator::ObjDetInstance->deinit();

    ServiceLocator::ObjDetInstance->unloadModel();

    Phrase group;
    if(detections.empty()){
        group = Phrase::WhatsThisNone;
    }else if(detections.size() == 1){
        group = Phrase::WhatsThisOne;
    }else{
        group = Phrase::WhatsThisTwo;
    }

    const int16_t id = Phrases::get(group);
    if(id < 0){
        CMF_LOG(WhatsThisRoutine, LogLevel::Warning, "Phrases::get returned no phrase id");
        return TickingState::Done;
    }

    const std::string templ = Phrases::map(group, id);

    char buf[256];
    if(group == Phrase::WhatsThisOne){
        const char* name = ObjDet::ClassNames.at(detections[0]);
        snprintf(buf, sizeof(buf), templ.c_str(), name, name);
    }else if(group == Phrase::WhatsThisTwo){
        const char* name1 = ObjDet::ClassNames.at(detections[0]);
        const char* name2 = ObjDet::ClassNames.at(detections[1]);
        snprintf(buf, sizeof(buf), templ.c_str(), name1, name2);
    }else{
        snprintf(buf, sizeof(buf), "%s", templ.c_str());
    }

    WhatsThisData data{};
    data.phrase = group;
    data.id = id;
    data.count = static_cast<uint8_t>(detections.size());
    data.class1 = detections.size() >= 1 ? static_cast<uint8_t>(detections[0]) : 0;
    data.class2 = detections.size() >= 2 ? static_cast<uint8_t>(detections[1]) : 0;
    com->sendData(BB::State::Scenario, BB::Action::Scenario::WhatsThis, data);

    auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, std::string(buf));
    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
    audio->waitEnd(portMAX_DELAY);

    return TickingState::Done;
}
