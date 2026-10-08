#ifndef CMF_EVENT_H
#define CMF_EVENT_H

#include <set>
#include <mutex>
#include "EventHandle.h"
#include "Memory/SmartPtr/WeakObjectPtr.h"
#include "Statics/ApplicationStatics.h"
#include "EventScanner.h"
#include "Core/Application.h"

/**
 * @brief Event class is used to declare event instances to which object function can be bound and executed as callbacks when the event is triggered.
 * @tparam Args Arguments being passed to each bound handle function.
 */
template<typename ...Args>
class Event {
public:
	/**
	 * @brief Deletes all contained bound handles and deallocates memory.
	 */
	inline virtual ~Event() noexcept{
		std::lock_guard guard(accessMutex);

		for(const HandleContainer& container : handles){
			delete container.handle;
		}
	}

	/**
	 * @brief Function for binding of handles via reference.
	 * @param handle The handle for a callback function being bound.
	 */
	inline void bind(EventHandle<Args...>&& handle) noexcept{
		if(handle.getOwningObject() == nullptr){
			return;
		}

		std::lock_guard guard(accessMutex);
		handles.insert({&handle, handle.getOwningObject(), false});
	}

	/**
	 * @brief Function for binding of handles via pointer.
	 * @param handle Pointer to the handle for a callback function being bound.
	 */
	inline void bind(EventHandle<Args...>* handle) noexcept{
		if(handle == nullptr){
			return;
		}

		if(handle->getOwningObject() == nullptr){
			return;
		}

		std::lock_guard guard(accessMutex);

		handles.insert({handle, handle->getOwningObject(), false});
	}

	/**
	 * @brief Function for binding a callback function directly via templated function reference and object pointer.
	 * @tparam O Type of Object the function is a member of.
	 * @tparam F The function type.
	 * @param object Object instance of which the function is being bound.
	 * @param function Function being bound.
	 */
	template<typename O, typename F>
	inline void bind(O* object, F&& function) noexcept requires (!std::constructible_from<std::function<void(Args...)>, F>){
		if(object == nullptr || function == nullptr){
			return;
		}

		std::lock_guard guard(accessMutex);

		HandleContainer container;
		container.handle = new EventHandle<Args...>();
		container.handle->bind(object, function);
		container.owningObject = object;

		handles.insert(container);
	}

	/**
	 * @brief Function for binding a callback function directly via std::function reference and object pointer.
	 * @param object Object instance of which the function is being bound.
	 * @param function Function being bound, in a std::function wrapper.
	 */
	template<typename F>
	inline void bind(Object* object, F&& function) noexcept requires std::constructible_from<std::function<void(Args...)>, F> {
		std::function<void(Args...)> func(function);

		if(object == nullptr || func == nullptr){
			return;
		}

		std::lock_guard guard(accessMutex);

		HandleContainer container;
		container.handle = new EventHandle<Args...>();
		container.handle->bind(object, func);
		container.owningObject = object;

		handles.insert(container);
	}

	/**
	 * @brief Function for removing bound function via object instance.
	 * @param object The object instance whose functions are to be removed.
	 */
	inline void unbind(Object* object) noexcept{
		std::lock_guard guard(accessMutex);

		std::erase_if(handles, [object](const HandleContainer& container){
			if(container.owningObject == object){
				delete container.handle;

				return true;
			}

			return false;
		});
	}

protected:
	/**
	 * @brief Internal broadcast function used by the derived classes. Calls all bound function callbacks with the given arguments.
	 * @param args Arguments passed to the bound function callbacks.
	 * @return True if all callback calls were successful, false otherwise.
	 */
	virtual inline bool _broadcast(const Args&... args) noexcept{
		std::lock_guard guard(accessMutex);

		bool succeeded = true;

		// TODO this should return false if app or event scanner are nullptr

		for(const HandleContainer& container : handles){
			if(container.handle == nullptr || !container.owningObject.isValid()){
				continue;
			}

			succeeded &= container.handle->call(args...);
		}

		if(const Application* app = ApplicationStatics::getApplication()){
			if(const EventScanner* scanner = app->getEventScanner()){
				scanner->unlock();
			}
		}

		return succeeded;
	}

private:
	/**
	 * @brief The internally used handle container which consists of the pointer to the object,
	 * the handle of event containing the callback function, and ownership flag.
	 */
	struct HandleContainer {
		EventHandle<Args...>* handle = nullptr;
		WeakObjectPtr<Object> owningObject = nullptr;

		// This is only needed for std::set to work
		/**
		 * @brief Less operator.
		 * @param other The HandleContainer being compared to.
		 * @return True if handle pointer is less than the other containers handle pointer.
		 */
		bool operator < (const HandleContainer& other) const noexcept{
			return (uint32_t) handle < (uint32_t) other.handle;
		}
	};

	std::set<HandleContainer> handles;
	std::mutex accessMutex;
};

#endif //CMF_EVENT_H