#ifndef CMF_EVENTBROADCASTER_H
#define CMF_EVENTBROADCASTER_H

#include "Event.h"
#include "Memory/SmartPtr/WeakObjectPtr.h"

/**
 * @brief Event broadcaster is a type of event, where only the owner of the event can trigger the broadcast.
 * @tparam Args The types of arguments being broadcast to the callback functions.
 */
template<typename ...Args>
class EventBroadcaster : public Event<Args...> {
public:
	/**
	 * @brief Deleted default constructor.
	 */
	inline EventBroadcaster() noexcept = delete;

	/**
	 * @brief Deleted copy constructor.
	 */
	inline EventBroadcaster(const EventBroadcaster&) noexcept = delete;

	/**
	 * @brief Deleted move constructor.
	 */
	inline EventBroadcaster(EventBroadcaster&&) noexcept = delete;

	/**
	 * @brief Constructor from a pointer to the owning object. Only this object can broadcast this event.
	 * @param owningObject The object owning the event.
	 */
	inline explicit EventBroadcaster(Object* owningObject) noexcept : owningObject(owningObject){}

protected:
	/**
	 * @brief Blocking broadcast implementation, ensuring that only the owner of the event can trigger a broadcast.
	 * @param args The arguments being broadcast.
	 * @param caller The caller object.
	 * @return True if successful, false otherwise.
	 */
	inline bool blockingBroadcast(const Args&... args, const Object* caller = nullptr) noexcept{
		if(caller != owningObject){
			return false;
		}

		return _broadcast(args...);
	}

private:
	virtual inline bool _broadcast(const Args&... args) noexcept override{
		return Event<Args...>::_broadcast(args...);
	}

private:
	WeakObjectPtr<Object> owningObject;
};

/**
 * @brief Broadcast macro with maximum wait time possible used instead of direct functions to ensure the object being called from is passed to the broadcast.
 */
#define broadcast(...) blockingBroadcast(__VA_ARGS__ __VA_OPT__(,) this)

/**
 * @brief Macro used to declare custom events as a member variable of an object, ensures that only the owning object type has visibility to the owning object type.
 * @param EventName The name of the event type.
 * @param OwningType The name of the owning object type.
 */
#define DECLARE_EVENT(EventName, OwningType, ...)																\
class EventName : public EventBroadcaster<__VA_ARGS__> {														\
	friend class OwningType;																					\
	inline explicit EventName(Object* owner) noexcept : EventBroadcaster<__VA_ARGS__>(owner) {}					\
};																												\

#endif //CMF_EVENTBROADCASTER_H