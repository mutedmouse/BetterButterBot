#ifndef BUTTERBOT_FIRMWARE_EVENTBAG_H
#define BUTTERBOT_FIRMWARE_EVENTBAG_H

#include <CtrlData.h>
#include <Scenarios.h>
#include <Object/Object.h>
#include <Event/Event.h>
#include <Services/Modules/ModuleType.h>
#include <Services/Modules/ModuleService.h>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include "Battery/Battery.h"
#include "Phone/Phone.h"

class EventBag : public Object {
	GENERATED_BODY(EventBag, Object, void)

public:
	void postInitProperties() noexcept override;

	enum class EventType : uint8_t {
		None,
		IncomingCall,
		Battery,
		Notification,
		Gas,
		Intruder,
		ModuleInsert,
		ModuleRemove,
		GasModuleConfigured,
		Phone,
		Summon,
		Scenario,
		COUNT
	};

	struct IncomingCallData {
		std::string caller;
	};

	struct BatteryData {
		Battery::Level level;
		ChargingState chargingState;
	};

	struct NotificationData {
		std::string message;
	};

	struct GasData {
		int16_t value = 0;
		bool ok = false;
	};

	struct IntruderData {
		bool detected = false;
	};

	struct ModuleData {
		Modules::Type type = Modules::Type::Unknown;
	};

	struct PhoneData {
		bool connected = false;
	};

	struct ScenarioEventData {
		BB::Action::Scenario scenario;
		ScenarioData data;
	};

	using EventPayload = std::variant<
		std::monostate,
		IncomingCallData,
		BatteryData,
		NotificationData,
		GasData,
		IntruderData,
		ModuleData,
		PhoneData,
		ScenarioEventData
	>;

	struct EventData {
		EventType type = EventType::None;
		EventPayload payload = std::monostate{};
	};

	// outEvents clears inside function
	void getEvents(std::vector<EventData>& outEvents);

	void start();
	void stop();
	void clear();
	EventData pop();
	bool probe() const;

	void startIntruderMonitoring();
	void stopIntruderMonitoring();

private:
	std::vector<EventData> events;

	// Low battery reporting latch
	bool lowBatteryReported = false;

	// Lowest value = highest priority
	static const std::map<EventType, uint8_t> priorityMap;

	static uint8_t priorityOf(EventType type);
	static bool isModuleEvent(EventType type);
	static bool isClearedByModuleRemove(EventType type);
	static void keepOnlyLastOfType(std::vector<EventData>& events, EventType type);
	static void keepOnlyLastModule(std::vector<EventData>& events);
	static void removeEarlierOnModuleRemove(std::vector<EventData>& events);
	template<typename PayloadT, typename FieldGetter>
	static void cancelOutPairs(std::vector<EventData>& events, EventType type, FieldGetter getField);
	static void cancelOutGas(std::vector<EventData>& events);
	static void cancelOutIntruder(std::vector<EventData>& events);
	static void cancelOutPhone(std::vector<EventData>& events);

	// Handlers
	void onIncomingCall(const char* caller);
	void onBatteryCharging(ChargingState chargingState);
	void onBatteryLevel(Battery::Level level);
	void onNotification(const char* message);
	void onGas(int16_t value, bool over);
	void onIntruder(bool detected);
	void onModule(uint8_t bus, Modules::Type type, ModuleService::Action action);
	void onCommand(Ctrl::Command command);
	void onScenario(BB::Action::Scenario scenario, ScenarioData data);
	void onGasModuleConfigured();
	void onPhoneConn(Phone::ConnEvent event);
};

#endif //BUTTERBOT_FIRMWARE_EVENTBAG_H
