#ifndef CMF_ENTRYPOINT_H
#define CMF_ENTRYPOINT_H

#include <cstdio>
#include <concepts>
#include "Core.h"
#include "Misc/Djb.h"
#include "Object/Object.h"
#include "Application.h"
#include "Memory/SmartPtr/StrongObjectPtr.h"
#include "Memory/ObjectMemory.h"
#include "Memory/GarbageCollector.h"
#include "Containers/Queue.h"
#include "Event/EventHandle.h"
#include "Log/Log.h"

/**
 * @brief The static CMF class used to setup the framework for execution and start the application execution.
 */
class CMF {
public:
	/**
	 * @brief Starts the application execution, sets up the garbage collector.
	 * @tparam T The type of application defined by the user for initialization and execution.
	 */
	template<typename T, typename = std::enable_if<std::derived_from<T, Application>>::type>
	static void start(){
		App = newObject<T>();
		if(!App.isValid()){
			CMF_LOG(CMF, Error, "Application instance could not be created.");
			return;
		}

		SubclassOf<GarbageCollector> garbageCollectorClass = App->getGarbageCollectorClass();
		if(garbageCollectorClass == nullptr){
			TrashCollector = newObject<GarbageCollector>(*App);
		}else{
			TrashCollector = newObject<GarbageCollector>(*garbageCollectorClass);
		}

		if(!TrashCollector.isValid()){
			CMF_LOG(CMF, Error, "GarbageCollector instance could not be created.");
		}
	}

private:
	inline static StrongObjectPtr<Application> App = nullptr;
	inline static StrongObjectPtr<GarbageCollector> TrashCollector = nullptr;
};

/**
 * @brief The macro abstracting the definition of the main function from the user of the framework.
 * @param AppName The name of the application type the user wants the framework to run.
 */
#define CMF_MAIN(AppName) 				\
	extern "C" void app_main() 			\
	{ 									\
		CMF::start<AppName>();      	\
		vTaskDelete(nullptr);			\
	}									\

#endif //CMF_ENTRYPOINT_H
