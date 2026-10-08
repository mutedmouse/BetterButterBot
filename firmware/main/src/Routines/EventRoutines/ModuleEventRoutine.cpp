#include "ModuleEventRoutine.h"
#include <Util/ServiceLocator.h>
#include <Core/Application.h>
#include <Statics/ApplicationStatics.h>
#include "Services/Settings.h"
#include "Phrases.h"
#include "Services/Com.h"
#include "Services/LED/LED.h"
#include "Enums.h"

DEFINE_LOG(ModuleEventRoutine)

const std::map<Modules::Type, Phrase> ModuleEventRoutine::ModuleInsertMapping = {
	{ Modules::Type::RM_CO2, Phrase::GasModuleInsert },
	{ Modules::Type::RM_IR, Phrase::IRModuleInsert },
	{ Modules::Type::RM_PerfBoard, Phrase::PerfModuleInsert },
	{ Modules::Type::RM_Motion, Phrase::PIRModuleInsert },
	{ Modules::Type::RM_LED, Phrase::LEDModuleInsert },
	{ Modules::Type::RM_TempHum, Phrase::TempHumModuleInsert },
	{ Modules::Type::Unknown, Phrase::UnknownModuleInsert },
};

const std::map<Modules::Type, Phrase> ModuleEventRoutine::ModuleRemoveMapping = {
	{ Modules::Type::RM_CO2, Phrase::GasModuleRemove },
	{ Modules::Type::RM_IR, Phrase::IRModuleRemove },
	{ Modules::Type::RM_PerfBoard, Phrase::PerfModuleRemove },
	{ Modules::Type::RM_Motion, Phrase::PIRModuleRemove },
	{ Modules::Type::RM_LED, Phrase::LEDModuleRemove },
	{ Modules::Type::RM_TempHum, Phrase::TempHumModuleRemove },
	{ Modules::Type::Unknown, Phrase::UnknownModuleRemove },
};


ModuleEventRoutine::ModuleEventRoutine(BBStateMachine* sm, EventBag::EventData data) : PhraseEventRoutine(sm, data){
	Application* app = ApplicationStatics::getApplication();

	const EventBag::ModuleData* moduleData = std::get_if<EventBag::ModuleData>(&data.payload);
	if(moduleData == nullptr){
		CMF_LOG(ModuleEventRoutine, LogLevel::Error, "Event payload is not ModuleData");
		return;
	}

	Phrase phrase = getPhraseCategory();
	if(phrase == Phrase::COUNT || phrase == Phrase::None){
		CMF_LOG(ModuleEventRoutine, LogLevel::Warning, "No phrase mapped for module event");
		return;
	}

	phraseID = Phrases::get(phrase);

	Com* com = app->getService<Com>();
	if(com == nullptr){
		CMF_LOG(ModuleEventRoutine, LogLevel::Error, "Com service missing!");
		return;
	}

	if(moduleData->type == Modules::Type::RM_CO2 && data.type == EventBag::EventType::ModuleInsert){
		if(ServiceLocator::SettingsInstance){
			if(!ServiceLocator::SettingsInstance->get().gasModuleConfigured){
				com->sendData(BB::State::Idle, BB::Action::Idle::GasConfigureStart, GasConfigureStartData{ .id = static_cast<uint8_t>(phraseID) });
				return;
			}
		}
	}

	com->sendData(BB::State::Idle, BB::Action::Idle::ModuleChange, ModuleData{ .id = static_cast<uint8_t>(phraseID), .type = static_cast<ModuleType>(moduleData->type), .inserted = data.type == EventBag::EventType::ModuleInsert });

	if(data.type == EventBag::EventType::ModuleRemove && moduleData->type == Modules::Type::RM_Motion){
		LED<MonoLED, RGBLED>* ledService = app->getService<LED<MonoLED, RGBLED>>();
		if(ledService) ledService->off(MonoLED::PIRIndicator);
	}
}

Routine::TickingState ModuleEventRoutine::tick(float deltaTime){
	if(data.type != EventBag::EventType::ModuleInsert && data.type != EventBag::EventType::ModuleRemove){
		return TickingState::Done;
	}

	return PhraseEventRoutine::tick(deltaTime);
}

int16_t ModuleEventRoutine::getPhraseID(){
	return phraseID;
}

Phrase ModuleEventRoutine::getPhraseCategory(){
	const EventBag::ModuleData* moduleData = std::get_if<EventBag::ModuleData>(&data.payload);
	if(moduleData == nullptr){
		CMF_LOG(ModuleEventRoutine, LogLevel::Warning, "getPhraseCategory: payload is not ModuleData");
		return Phrase::None;
	}

	if(data.type == EventBag::EventType::ModuleInsert){
		if(moduleData->type == Modules::Type::RM_CO2){
			const Application* app = Application::getApp();
			if(ServiceLocator::SettingsInstance){
				if(!ServiceLocator::SettingsInstance->get().gasModuleConfigured){
					return Phrase::GasModuleFirstInsert;
				}
			}
		}

		if(ModuleInsertMapping.contains(moduleData->type)){
			return ModuleInsertMapping.at(moduleData->type);
		}
	}

	if(data.type == EventBag::EventType::ModuleRemove && ModuleRemoveMapping.contains(moduleData->type)){
		return ModuleRemoveMapping.at(moduleData->type);
	}

	return Phrase::None;
}

SpeechGen::InputType ModuleEventRoutine::getPhraseType(){
	return SpeechGen::InputType::Text;
}
