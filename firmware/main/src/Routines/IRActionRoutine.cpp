#include "IRActionRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Services/Modules/ModuleService.h>
#include <Services/Modules/ModuleDevices/RM_IRModule.h>
#include <BBData.h>
#include <Phrases.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/Com.h"
#include "Services/IRStorage.h"

DEFINE_LOG(IRActionRoutine)

Routine::TickingState IRActionRoutine::tick(float deltaTime){
    const Application* app = ApplicationStatics::getApplication();
    Audio* audio = app->getService<Audio>();
    Com* com = app->getService<Com>();
    ModuleService* modules = app->getService<ModuleService>();

    if(!audio || !ServiceLocator::SpeechAudioGenInstance || !ServiceLocator::SpeechGenInstance || !com || !modules || !ServiceLocator::IRStorageInstance){
        CMF_LOG(IRActionRoutine, LogLevel::Error, "Missing required service(s)");
        return TickingState::Done;
    }

    if(modules->getInserted() != Modules::Type::RM_IR){
        CMF_LOG(IRActionRoutine, LogLevel::Warning, "IR module not inserted");
        return TickingState::Done;
    }

    RM_IRModule* irModule = cast<RM_IRModule>(modules->getDevice().get());
    if(!irModule){
        return TickingState::Done;
    }

    const uint8_t index = ServiceLocator::IRStorageInstance->getSelectedIndex();
    if(index >= ServiceLocator::IRStorageInstance->getCount()){
        CMF_LOG(IRActionRoutine, LogLevel::Warning, "Selected IR index out of range");
        return TickingState::Done;
    }

    const IRCommand& cmd = ServiceLocator::IRStorageInstance->getCommand(index);

    irModule->sendIR(cmd.frame, cmd.frameCount, cmd.sendCount, cmd.periodUs, cmd.carrierHz, cmd.carrierDutyPct);

    {
        IR_actionData data{};
        data.commandIndex = index;
        com->sendData(BB::State::Scenario, BB::Action::Scenario::IR_action, data);
    }

    const int16_t id = Phrases::get(Phrase::IRActionDone);
    if(id >= 0){
        const std::string text = Phrases::map(Phrase::IRActionDone, id);
        auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, text);
        audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
        audio->waitEnd(portMAX_DELAY);
    }

    return TickingState::Done;
}
