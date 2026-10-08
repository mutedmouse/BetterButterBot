#include "ObserveRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <BBData.h>
#include <Phrases.h>
#include <algorithm>
#include <Util/ServiceLocator.h>
#include "Audio/AudioFrontend.h"
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/ObjDet.h"

DEFINE_LOG(ObserveRoutine)

namespace {

constexpr Phrase singlePhrase(ObjClass cls){
    switch(cls){
        case ObjClass::Backpack:   return Phrase::ObserveBackpack;
        case ObjClass::Bottle:     return Phrase::ObserveBottle;
        case ObjClass::Controller: return Phrase::ObserveController;
        case ObjClass::Keyboard:   return Phrase::ObserveKeyboard;
        case ObjClass::Lamp:       return Phrase::ObserveLamp;
        case ObjClass::Laptop:     return Phrase::ObserveLaptop;
        case ObjClass::Mug:        return Phrase::ObserveMug;
        case ObjClass::Notebook:   return Phrase::ObserveNotebook;
        case ObjClass::Phone:      return Phrase::ObservePhone;
        case ObjClass::Plant:      return Phrase::ObservePlant;
        default:                 return Phrase::ObserveNone;
    }
}

constexpr uint16_t pairKey(ObjClass a, ObjClass b){
    return static_cast<uint16_t>(a) * static_cast<uint16_t>(ObjClass::COUNT) + static_cast<uint16_t>(b);
}

Phrase pairPhrase(ObjClass a, ObjClass b){
    if(static_cast<int>(a) > static_cast<int>(b)){
        std::swap(a, b);
    }

    switch(pairKey(a, b)){
        case pairKey(ObjClass::Backpack,   ObjClass::Bottle):     return Phrase::ObserveBackpackBottle;
        case pairKey(ObjClass::Backpack,   ObjClass::Laptop):     return Phrase::ObserveBackpackLaptop;
        case pairKey(ObjClass::Controller, ObjClass::Laptop):     return Phrase::ObserveControllerLaptop;
        case pairKey(ObjClass::Controller, ObjClass::Phone):      return Phrase::ObserveControllerPhone;
        case pairKey(ObjClass::Keyboard,   ObjClass::Mug):        return Phrase::ObserveKeyboardMug;
        case pairKey(ObjClass::Laptop,     ObjClass::Mug):        return Phrase::ObserveLaptopMug;
        case pairKey(ObjClass::Laptop,     ObjClass::Phone):      return Phrase::ObserveLaptopPhone;
        case pairKey(ObjClass::Laptop,     ObjClass::Plant):      return Phrase::ObservePlantLaptop;
        case pairKey(ObjClass::Mug,        ObjClass::Plant):      return Phrase::ObservePlantMug;
        case pairKey(ObjClass::Notebook,   ObjClass::Phone):      return Phrase::ObserveNotebookPhone;
        default:                                              return Phrase::None;
    }
}

}

Routine::TickingState ObserveRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();

    if(audio == nullptr || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || com == nullptr || !ServiceLocator::ObjDetInstance){
        CMF_LOG(ObserveRoutine, LogLevel::Error, "Missing required service(s)");
        abort();
    }

    ServiceLocator::ObjDetInstance->loadModel();

    ServiceLocator::ObjDetInstance->init();
    const std::vector<ObjClass> detections = ServiceLocator::ObjDetInstance->detect();
    ServiceLocator::ObjDetInstance->deinit();

    ServiceLocator::ObjDetInstance->unloadModel();

    Phrase group;
    if(detections.empty()){
        group = Phrase::ObserveNone;
    }else if(detections.size() == 1){
        group = singlePhrase(detections[0]);
    }else{
        group = pairPhrase(detections[0], detections[1]);
        if(group == Phrase::None){
            group = singlePhrase(detections[0]);
        }
    }

    const int16_t id = Phrases::get(group);
    if(id < 0){
        CMF_LOG(ObserveRoutine, LogLevel::Warning, "Phrases::get returned no phrase id");
        return TickingState::Done;
    }

    ObserveData data{};
    data.phrase = group;
    data.id = id;
    data.count = static_cast<uint8_t>(detections.size());
    data.class1 = detections.size() >= 1 ? static_cast<uint8_t>(detections[0]) : 0;
    data.class2 = detections.size() >= 2 ? static_cast<uint8_t>(detections[1]) : 0;
    com->sendData(BB::State::Idle, BB::Action::Idle::Observe, data);

    auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(group, id));
    audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
    audio->waitEnd(portMAX_DELAY);

    return TickingState::Done;
}
