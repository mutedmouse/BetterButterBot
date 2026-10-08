#include "PowerOffRoutine.h"
#include "Services/ShutdownService.h"

Routine::TickingState PowerOffRoutine::tick(float deltaTime){
	ShutdownService::Shutdown(ShutdownReason::Command);
	vTaskDelay(2000);

	return TickingState::Done;
}
