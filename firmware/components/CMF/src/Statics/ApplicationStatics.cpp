#include "ApplicationStatics.h"
#include "Core/Application.h"
#include "Log/Log.h"

DEFINE_LOG(ApplicationStatics);

Application* ApplicationStatics::getApplication() noexcept{
	if(!isValid(Application::ApplicationInstance)){
		CMF_LOG(ApplicationStatics, LogLevel::Error, "Application instance is invalid.");
		abort();
	}

	return Application::ApplicationInstance;
}
