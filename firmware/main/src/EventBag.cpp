#include "EventBag.h"
#include <algorithm>
#include <Services/GasConfigureService.h>
#include "Services/Com.h"
#include "Services/Modules/ModuleService.h"
#include "Phone/Phone.h"

DEFINE_LOG(EventBag)

const std::map<EventBag::EventType, uint8_t> EventBag::priorityMap = {
	{ EventBag::EventType::IncomingCall, 1 },
	{ EventBag::EventType::Scenario, 2 },
	{ EventBag::EventType::Summon, 3 },
	{ EventBag::EventType::Battery, 3 },
	{ EventBag::EventType::Notification, 4 },
	{ EventBag::EventType::GasModuleConfigured, 4 },
	{ EventBag::EventType::Phone, 4 },
	{ EventBag::EventType::ModuleRemove, 5 },
	{ EventBag::EventType::ModuleInsert, 5 },
	{ EventBag::EventType::Gas, 6 },
	{ EventBag::EventType::Intruder, 6 },
};

uint8_t EventBag::priorityOf(EventType type){
	std::map<EventType, uint8_t>::const_iterator it = priorityMap.find(type);
	if(it == priorityMap.end()){
		return UINT8_MAX;
	}
	return it->second;
}

bool EventBag::isModuleEvent(EventType type){
	return type == EventType::ModuleInsert || type == EventType::ModuleRemove;
}

bool EventBag::isClearedByModuleRemove(EventType type){
	return type == EventType::ModuleInsert
		|| type == EventType::Gas
		|| type == EventType::Intruder;
}

void EventBag::keepOnlyLastOfType(std::vector<EventData>& events, EventType type){
	const size_t total = std::count_if(events.begin(), events.end(),
		[type](const EventData& e){ return e.type == type; });
	if(total <= 1){
		return;
	}

	size_t seen = 0;
	events.erase(std::remove_if(events.begin(), events.end(),
		[type, total, &seen](const EventData& e){
			if(e.type != type){
				return false;
			}
			++seen;
			return seen < total;
		}), events.end());
}

void EventBag::keepOnlyLastModule(std::vector<EventData>& events){
	const size_t total = std::count_if(events.begin(), events.end(),
		[](const EventData& e){ return isModuleEvent(e.type); });
	if(total <= 1){
		return;
	}

	size_t seen = 0;
	events.erase(std::remove_if(events.begin(), events.end(),
		[total, &seen](const EventData& e){
			if(!isModuleEvent(e.type)){
				return false;
			}
			++seen;
			return seen < total;
		}), events.end());
}

void EventBag::removeEarlierOnModuleRemove(std::vector<EventData>& events){
	bool found = false;
	size_t moduleRemoveIdx = 0;
	for(size_t i = 0; i < events.size(); ++i){
		if(events[i].type == EventType::ModuleRemove){
			moduleRemoveIdx = i;
			found = true;
			break;
		}
	}
	if(!found){
		return;
	}

	std::vector<EventData> filtered;
	filtered.reserve(events.size());
	for(size_t i = 0; i < events.size(); ++i){
		if(i < moduleRemoveIdx && isClearedByModuleRemove(events[i].type)){
			continue;
		}
		filtered.emplace_back(std::move(events[i]));
	}
	events = std::move(filtered);
}

template<typename PayloadT, typename FieldGetter>
void EventBag::cancelOutPairs(std::vector<EventData>& events, EventType type, FieldGetter getField){
	std::vector<size_t> stack;
	std::vector<bool> remove(events.size(), false);

	for(size_t i = 0; i < events.size(); ++i){
		if(events[i].type != type){
			continue;
		}
		const PayloadT* data = std::get_if<PayloadT>(&events[i].payload);
		if(data == nullptr){
			remove[i] = true;
			continue;
		}
		if(!stack.empty()){
			const PayloadT* topData = std::get_if<PayloadT>(&events[stack.back()].payload);
			if(topData != nullptr && getField(*topData) != getField(*data)){
				remove[stack.back()] = true;
				remove[i] = true;
				stack.pop_back();
				continue;
			}
		}
		stack.emplace_back(i);
	}

	// Of un-cancelled events, keep only the last
	for(size_t i = 0; i + 1 < stack.size(); ++i){
		remove[stack[i]] = true;
	}

	std::vector<EventData> filtered;
	filtered.reserve(events.size());
	for(size_t i = 0; i < events.size(); ++i){
		if(!remove[i]){
			filtered.emplace_back(std::move(events[i]));
		}
	}
	events = std::move(filtered);
}

void EventBag::cancelOutGas(std::vector<EventData>& events){
	cancelOutPairs<GasData>(events, EventType::Gas,
		[](const GasData& d){ return d.ok; });
}

void EventBag::cancelOutIntruder(std::vector<EventData>& events){
	cancelOutPairs<IntruderData>(events, EventType::Intruder,
		[](const IntruderData& d){ return d.detected; });
}

void EventBag::cancelOutPhone(std::vector<EventData>& events){
	cancelOutPairs<PhoneData>(events, EventType::Phone,
		[](const PhoneData& d){ return d.connected; });
}

void EventBag::postInitProperties() noexcept{
	start();
}

void EventBag::getEvents(std::vector<EventData>& outEvents){
	outEvents.clear();
	if(events.empty()){
		return;
	}

	// Filtering rules in order:
	// 1. IncomingCall and Notification are never removed.
	// 2. Keep only the last Battery.
	keepOnlyLastOfType(events, EventType::Battery);
	// 3. Among ModuleInsert/ModuleRemove, keep only the last.
	keepOnlyLastModule(events);
	// 4. Gas events cancel out on opposing 'over'; keep only the last un-cancelled.
	cancelOutGas(events);
	// 5. Intruder events cancel out on opposing 'detected'; keep only the last un-cancelled.
	cancelOutIntruder(events);
	// 6. Phone connect/disconnect events cancel out; keep only the last un-cancelled.
	cancelOutPhone(events);
	// 7. Keep only the last Summon; repeated summons collapse into a single search.
	keepOnlyLastOfType(events, EventType::Summon);
	// 8. Keep only the last Scenario; a newer scenario trigger replaces older ones.
	keepOnlyLastOfType(events, EventType::Scenario);

	outEvents.reserve(events.size());
	for(EventData& event : events){
		outEvents.emplace_back(std::move(event));
	}
	events.clear();

	// Sort outEvents by priority (stable to preserve chronological order within same priority).
	std::stable_sort(outEvents.begin(), outEvents.end(),
		[](const EventData& a, const EventData& b){
			return priorityOf(a.type) < priorityOf(b.type);
		});
}

void EventBag::start(){
	const Application* app = getApp();

	ModuleService* moduleService = app->getService<ModuleService>();
	if(moduleService == nullptr){
		CMF_LOG(EventBag, LogLevel::Error, "Module service is null!");
		return;
	}

	moduleService->ModulesEvent.bind(this, &EventBag::onModule);

	Battery* battery = app->getService<Battery>();
	if(battery == nullptr){
		CMF_LOG(EventBag, LogLevel::Error, "Battery service is null!");
		return;
	}

	battery->OnLevelChanged.bind(this, &EventBag::onBatteryLevel);
	battery->OnChargeStatus.bind(this, &EventBag::onBatteryCharging);

	auto com = app->getService<Com>();
	if(com == nullptr){
		CMF_LOG(EventBag, LogLevel::Error, "Com service is null!");
		return;
	}
	com->OnCommand.bind(this, &EventBag::onCommand);
	com->OnScenario.bind(this, &EventBag::onScenario);

	if(GasConfigureService* gasService = app->getService<GasConfigureService>()){
		gasService->OnGasConfigureDone.bind(this, &EventBag::onGasModuleConfigured);
	}

	Phone* phone = app->getService<Phone>();
	if(phone == nullptr){
		CMF_LOG(EventBag, LogLevel::Error, "Phone service is null!");
		return;
	}
	phone->onConnEvent.bind(this, &EventBag::onPhoneConn);
}

void EventBag::stop(){
	const Application* app = getApp();

	ModuleService* moduleService = app->getService<ModuleService>();
	if(moduleService == nullptr){
		CMF_LOG(EventBag, LogLevel::Warning, "Module service is null on stop!");
		return;
	}

	moduleService->ModulesEvent.unbind(this);

	Battery* battery = app->getService<Battery>();
	if(battery == nullptr){
		CMF_LOG(EventBag, LogLevel::Warning, "Battery service is null on stop!");
		return;
	}

	battery->OnLevelChanged.unbind(this);
	battery->OnChargeStatus.unbind(this);

	if(GasConfigureService* gasService = app->getService<GasConfigureService>()){
		gasService->OnGasConfigureDone.unbind(this);
	}

	stopIntruderMonitoring();

	Phone* phone = app->getService<Phone>();
	if(phone){
		phone->onConnEvent.unbind(this);
	}else{
		CMF_LOG(EventBag, LogLevel::Warning, "Phone service is null on stop!");
	}
}

void EventBag::clear(){
	events.clear();
}

bool EventBag::probe() const{
	if(events.empty()){
		return false;
	}

	// Mirrors pop() filtering. Any non-Gas/Intruder event is guaranteed to leave at
	// least one survivor (IncomingCall/Notification are never filtered;
	// keepOnlyLast keeps Battery; module events always leave at least one).
	// Only Gas and Intruder events can fully cancel out via cancelOutPairs.
	// The cancelOutPairs stack is homogeneous in field value, so we track it as
	// a (net, value) pair instead of materialising the full stack.
	int gasNet = 0;
	bool gasValue = false;
	int intruderNet = 0;
	bool intruderValue = false;
	int phoneNet = 0;
	bool phoneValue = false;

	for(const EventData& e : events){
		if(e.type == EventType::Gas){
			const GasData* d = std::get_if<GasData>(&e.payload);
			if(d == nullptr){
				continue;
			}
			if(gasNet == 0){
				gasNet = 1;
				gasValue = d->ok;
			}else if(d->ok == gasValue){
				++gasNet;
			}else{
				--gasNet;
			}
		}else if(e.type == EventType::Intruder){
			const IntruderData* d = std::get_if<IntruderData>(&e.payload);
			if(d == nullptr){
				continue;
			}
			if(intruderNet == 0){
				intruderNet = 1;
				intruderValue = d->detected;
			}else if(d->detected == intruderValue){
				++intruderNet;
			}else{
				--intruderNet;
			}
		}else if(e.type == EventType::Phone){
			const PhoneData* d = std::get_if<PhoneData>(&e.payload);
			if(d == nullptr){
				continue;
			}
			if(phoneNet == 0){
				phoneNet = 1;
				phoneValue = d->connected;
			}else if(d->connected == phoneValue){
				++phoneNet;
			}else{
				--phoneNet;
			}
		}else{
			return true;
		}
	}

	return gasNet > 0 || intruderNet > 0 || phoneNet > 0;
}

EventBag::EventData EventBag::pop(){
	if(events.empty()){
		EventData none;
		none.type = EventType::None;
		return none;
	}

	// Filtering rules in order:
	// 1. IncomingCall and Notification are never removed.
	// 2. Keep only the last Battery.
	keepOnlyLastOfType(events, EventType::Battery);
	// 3. Among ModuleInsert/ModuleRemove, keep only the last.
	keepOnlyLastModule(events);
	// 4. If a ModuleRemove exists, drop all earlier ModuleInsert/Gas/Intruder/Motion events.
	removeEarlierOnModuleRemove(events);
	// 5. Gas events cancel out on opposing 'over'; keep only the last un-cancelled.
	cancelOutGas(events);
	// 6. Intruder events cancel out on opposing 'detected'; keep only the last un-cancelled.
	cancelOutIntruder(events);
	// 7. Phone connect/disconnect events cancel out; keep only the last un-cancelled.
	cancelOutPhone(events);
	// 8. Keep only the last Summon; repeated summons collapse into a single search.
	keepOnlyLastOfType(events, EventType::Summon);
	// 9. Keep only the last Scenario; a newer scenario trigger replaces older ones.
	keepOnlyLastOfType(events, EventType::Scenario);

	if(events.empty()){
		EventData none;
		none.type = EventType::None;
		return none;
	}

	std::vector<EventData>::iterator highestPriorityIt = events.begin();
	uint8_t highestPriority = priorityOf(highestPriorityIt->type);
	if(!priorityMap.contains(highestPriorityIt->type)){
		CMF_LOG(EventBag, LogLevel::Error, "PriorityMap missing type: %d", static_cast<int>(highestPriorityIt->type));
	}

	for(std::vector<EventData>::iterator it = events.begin(); it != events.end(); ++it){
		uint8_t currentPriority = priorityOf(it->type);
		if(currentPriority < highestPriority){
			highestPriority = currentPriority;
			highestPriorityIt = it;
		}
	}

	EventData result = std::move(*highestPriorityIt);
	events.erase(highestPriorityIt);

	return result;
}

// Handlers
void EventBag::onIncomingCall(const char* caller){
	EventData data{};
	data.type = EventType::IncomingCall;
	data.payload = IncomingCallData{ caller ? caller : "" };
	events.push_back(std::move(data));
}

void EventBag::onBatteryCharging(ChargingState chargingState){
	if(chargingState == ChargingState::Unplugged){
		return; // Only collect the events that will have a response routine
	}

	lowBatteryReported = false;

	EventData data{};
	data.type = EventType::Battery;
	data.payload = BatteryData{ Battery::Level::COUNT, chargingState };
	events.push_back(std::move(data));
}

void EventBag::onBatteryLevel(Battery::Level level){
	if(level != Battery::Level::Critical && level != Battery::Level::VeryLow){
		return; // Only collect the events that will have a response routine
	}

	if(level == Battery::Level::VeryLow){
		if(lowBatteryReported){
			return;
		}
		lowBatteryReported = true;
	}

	EventData data{};
	data.type = EventType::Battery;
	data.payload = BatteryData{ level, ChargingState::Unplugged }; // This is unplugged because there is no None
	events.push_back(std::move(data));
}

void EventBag::onNotification(const char* message){
	EventData data{};
	data.type = EventType::Notification;
	data.payload = NotificationData{ message ? message : "" };
	events.push_back(std::move(data));
}

void EventBag::onGas(int16_t value, bool over){
	EventData data{};
	data.type = EventType::Gas;
	data.payload = GasData{ value, over };
	events.push_back(std::move(data));
}

void EventBag::onIntruder(bool detected){
	EventData data{};
	data.type = EventType::Intruder;
	data.payload = IntruderData{ detected };
	events.push_back(std::move(data));
}

void EventBag::onModule(uint8_t bus, Modules::Type type, ModuleService::Action action){
	EventData data{};
	data.payload = ModuleData{ type };

	if(action == ModuleService::Action::Insert){
		data.type = EventType::ModuleInsert;
	}else{
		data.type = EventType::ModuleRemove;
	}

	if(action == ModuleService::Action::Remove && type == Modules::Type::RM_Motion){
		stopIntruderMonitoring();
	}

	events.push_back(std::move(data));
}

void EventBag::startIntruderMonitoring(){
	const Application* app = getApp();
	ModuleService* moduleService = app->getService<ModuleService>();
	if(moduleService == nullptr){
		CMF_LOG(EventBag, LogLevel::Error, "Module service is null on startIntruderMonitoring!");
		return;
	}

#ifdef CONFIG_RM_Motion
	moduleService->OnRM_Motion.bind(this, &EventBag::onIntruder);
#endif
}

void EventBag::stopIntruderMonitoring(){
	const Application* app = getApp();
	ModuleService* moduleService = app->getService<ModuleService>();
	if(moduleService == nullptr){
		CMF_LOG(EventBag, LogLevel::Warning, "Module service is null on stopIntruderMonitoring!");
		return;
	}

#ifdef CONFIG_RM_Motion
	moduleService->OnRM_Motion.unbind(this);
#endif
}

void EventBag::onCommand(Ctrl::Command command){
	if(command == Ctrl::Summon){
		EventData data{};
		data.type = EventType::Summon;
		events.push_back(std::move(data));
	}
}

void EventBag::onScenario(BB::Action::Scenario scenario, ScenarioData data){
	EventData event{};
	event.type = EventType::Scenario;
	event.payload = ScenarioEventData{ scenario, data };
	events.push_back(std::move(event));
}

void EventBag::onGasModuleConfigured(){
	EventData data{};
	data.type = EventType::GasModuleConfigured;
	events.push_back(std::move(data));
}

void EventBag::onPhoneConn(Phone::ConnEvent event){
	EventData data{};
	data.type = EventType::Phone;
	data.payload = PhoneData{ event == Phone::ConnEvent::Connected };
	events.push_back(std::move(data));
}
