#include "SummonEventRoutine.h"
#include "States/BBStateMachine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <Services/Audio/Audio.h>
#include <Util/stdafx.h>
#include <cstdlib>
#include <cmath>
#include <memory>
#include <Phrases.h>
#include <BBData.h>
#include <Util/ServiceLocator.h>
#include "Audio/SpeechAudioGen.h"
#include "Audio/SpeechAudioSource.h"
#include "Audio/SpeechGen.h"
#include "Services/BaseBoard.h"
#include "Services/MotionService.h"
#include "Services/Com.h"
#include "FaceDet.h"
#include "Services/VoiceLink.h"
#include <cstdarg>
#include <cstdio>

DEFINE_LOG(SummonEventRoutine)

SummonEventRoutine::~SummonEventRoutine(){
	const Application* app = ApplicationStatics::getApplication();

	if(BaseBoard* bb = app->getService<BaseBoard>()){
		bb->stopMotors();
	}

	if(faceDetActive){
		if(FaceDet* fd = app->getService<FaceDet>()){
			fd->stop();
			sm->unbindRoutine(fd->OnDetect);
			fd->unloadModel();
		}
	}
}

Routine::TickingState SummonEventRoutine::tick(float deltaTime){
	const Application* app = ApplicationStatics::getApplication();

	if(phase != Phase::Init && phase != Phase::ProxCheck && proxInterrupted){
		CMF_LOG(SummonEventRoutine, LogLevel::Info, "Unsafe proximity state - interrupting");
		static constexpr const char* PhaseNames[] = { "init", "start check", "looking", "turning", "centring", "driving", "settling" };
		note("find me: stopped by %s while %s (look %u of %u)", interruptCause,
			 PhaseNames[static_cast<uint8_t>(phase)], rotationCount + 1, MaxRotations + 1);
		baseBoard->stopMotors();
		speak(Phrase::RoutineInterrupted, SummonData::Phase::Interrupted);
		return TickingState::Done;
	}

	switch(phase){
		case Phase::Init: {
			baseBoard = app->getService<BaseBoard>();
			faceDet = app->getService<FaceDet>();
			MotionService* motionService = app->getService<MotionService>();

			if(baseBoard == nullptr || faceDet == nullptr || motionService == nullptr){
				CMF_LOG(SummonEventRoutine, LogLevel::Error, "Missing required service(s)");
				return TickingState::Done;
			}

			if(baseBoard->getChargingState() != ChargingState::Unplugged){
				CMF_LOG(SummonEventRoutine, LogLevel::Info, "Charging - skipping");
				note("find me: not moving, charging (USB or charger connected)");
				speak(Phrase::SummonCharging, SummonData::Phase::Charging);
				return TickingState::Done;
			}

			if(!motionService->isUprightOnGround()){
				CMF_LOG(SummonEventRoutine, LogLevel::Info, "Not upright on the ground - skipping");
				{
					const SC7A20::Sample a = motionService->lastUprightSample();
					note("find me: not moving, not upright (lifted=%d, accel x=%d y=%d z=%d in 1/100 g)",
						 motionService->isLifted(), (int) (a.accelX * 100), (int) (a.accelY * 100), (int) (a.accelZ * 100));
				}
				speak(Phrase::CannotMove, SummonData::Phase::Unsafe);
				return TickingState::Done;
			}

			const bool turnRight = rand() % 2;
			note("find me: start, will turn %s in 30-degree steps", turnRight ? "right" : "left");
			scanMotorLeft = turnRight ? RotateSpeed : -RotateSpeed;
			scanMotorRight = turnRight ? -RotateSpeed : RotateSpeed;

			sm->bindRoutine(baseBoard->onProximityReading, this, &SummonEventRoutine::onProximityReading);
			sm->bindRoutine(baseBoard->onProximityChange, this, &SummonEventRoutine::onProximityChange);
			sm->bindRoutine(motionService->onMotion, this, &SummonEventRoutine::onMotion);
			baseBoard->requestProximityState();
			proxDeadlineMs = millis() + ProxReadingTimeoutMs;
			phase = Phase::ProxCheck;
			return TickingState::Continue;
		}

		case Phase::ProxCheck: {
			if(proxUnsafe || proxInterrupted){
				CMF_LOG(SummonEventRoutine, LogLevel::Info, "Unsafe proximity state - skipping");
				note("find me: not moving, start check front=%s bottom=%s%s",
					 startFront == ProxState::Covered ? "close" : startFront == ProxState::Uncovered ? "clear" : "?",
					 startBottom == ProxState::Covered ? "floor" : startBottom == ProxState::Uncovered ? "NO FLOOR" : "?",
					 proxInterrupted ? " (changed during check)" : "");
				speak(Phrase::CannotMove, SummonData::Phase::Unsafe);
				return TickingState::Done;
			}
			if(retryAtMs != 0){
			if(millis() >= retryAtMs){
				retryAtMs = 0;
				baseBoard->requestProximityState();
				proxDeadlineMs = millis() + ProxReadingTimeoutMs;
			}
			return TickingState::Continue;
		}
		if(!proxReadingReceived && millis() < proxDeadlineMs){
				return TickingState::Continue;
			}

			faceDet->loadModel(true);   // long range: 320x240 + crops
			sm->bindRoutine(faceDet->OnDetect, this, &SummonEventRoutine::onDetect);
			faceDet->start(false);
			faceDetActive = true;

			detected = false;
			beginLook();
			speak(Phrase::SummonStart, SummonData::Phase::Start);

			scanDeadlineMs = millis() + ScanWindowMs;
			phase = Phase::Scanning;
			return TickingState::Continue;
		}

		case Phase::Scanning: {
			if(detected){
				note("find me: FACE on look %u: at x=%d%% (width %d%%)", rotationCount + 1, lookX, lookW);
				aimOrDrive();
				return TickingState::Continue;
			}

			if(millis() >= scanDeadlineMs){
				reportLook();
				if(aimCount > 0){
					// Turned toward the face but lost it: it was close to the middle, so go.
					note("find me: lost the face after aiming, driving");
					startDrive();
					return TickingState::Continue;
				}
				if(rotationCount >= MaxRotations){
					note("find me: nobody seen after a full turn");
					CMF_LOG(SummonEventRoutine, LogLevel::Info, "No face after full rotation");
					faceDet->stop();
					speak(Phrase::SummonNotFound, SummonData::Phase::NotFound);
					return TickingState::Done;
				}

				faceDet->stop();
				baseBoard->setMotors(scanMotorLeft, scanMotorRight);
				rotateDeadlineMs = millis() + RotateStepMs;
				phase = Phase::Rotating;
			}

			return TickingState::Continue;
		}

		case Phase::Rotating: {
			if(millis() >= rotateDeadlineMs){
				baseBoard->stopMotors();
				rotationCount++;
				settleUntilMs = millis() + SettleMs;   // let the robot stop rocking before looking
				phase = Phase::Settling;
			}

			return TickingState::Continue;
		}

		case Phase::Settling: {
			if(millis() >= settleUntilMs){
				faceDet->start(false);
				detected = false;
				beginLook();
				scanDeadlineMs = millis() + ScanWindowMs;
				phase = Phase::Scanning;
			}

			return TickingState::Continue;
		}

		case Phase::Centering: {
			// Timed aiming turn (see aimOrDrive); then settle and look again.
			if(millis() >= centerDeadlineMs){
				baseBoard->stopMotors();
				settleUntilMs = millis() + SettleMs;
				phase = Phase::Settling;
			}

			return TickingState::Continue;
		}

		case Phase::Driving: {
			if(millis() >= driveEndMs){
				baseBoard->stopMotors();
				faceDet->stop();
				note("find me: arrived");
				speak(Phrase::SummonFound, SummonData::Phase::Found);
				return TickingState::Done;
			}

			return TickingState::Continue;
		}
	}

	return TickingState::Done;
}

void SummonEventRoutine::aimOrDrive(){
	const float error = faceCenterX - 0.5f;
	if(std::fabs(error) <= CenterDeadband || aimCount >= MaxAims){
		note("find me: %s (face x=%d%%), driving", std::fabs(error) <= CenterDeadband ? "facing you" : "aim limit reached",
			 (int) (faceCenterX * 100));
		startDrive();
		return;
	}

	aimCount++;
	const float degrees = std::fabs(error) * CameraFovDeg;
	note("find me: aim %u: turning %d degrees %s toward the face", aimCount, (int) degrees, error > 0.0f ? "right" : "left");
	faceDet->stop();
	// A face on the right of the frame (error > 0) is to the robot's right, so turn right.
	if(error > 0.0f){
		baseBoard->setMotors(RotateSpeed, -RotateSpeed);
	} else{
		baseBoard->setMotors(-RotateSpeed, RotateSpeed);
	}
	centerDeadlineMs = millis() + static_cast<uint32_t>(degrees * AimMsPerDeg);
	phase = Phase::Centering;
}

void SummonEventRoutine::startDrive(){
	baseBoard->stopMotors();
	faceDet->stop();
	driveEndMs = millis() + DriveMinMs + static_cast<uint32_t>(rand() % (DriveMaxMs - DriveMinMs + 1));
	baseBoard->setMotors(DriveSpeed, DriveSpeed);
	phase = Phase::Driving;
}

void SummonEventRoutine::speak(Phrase phrase, SummonData::Phase dataPhase){
	const Application* app = ApplicationStatics::getApplication();
	Audio* audio = app->getService<Audio>();
	Com* com = app->getService<Com>();

	if(audio == nullptr || ServiceLocator::SpeechAudioGenInstance == nullptr || ServiceLocator::SpeechGenInstance == nullptr || com == nullptr){
		CMF_LOG(SummonEventRoutine, LogLevel::Error, "Missing required service(s)");
		return;
	}

	const int16_t id = Phrases::get(phrase);
	if(id < 0){
		CMF_LOG(SummonEventRoutine, LogLevel::Warning, "Phrases::get returned no phrase id");
		return;
	}

	SummonData data{};
	data.phase = dataPhase;
	data.id = static_cast<uint8_t>(id);
	com->sendData(BB::State::Idle, BB::Action::Idle::Summon, data);

	auto source = std::make_unique<SpeechAudioSource>(SpeechGen::InputType::Text, Phrases::map(phrase, id));
	audio->play(ServiceLocator::SpeechAudioGenInstance.get(), std::move(source));
	audio->waitEnd(portMAX_DELAY);
}

void SummonEventRoutine::onDetect(int faceCount, bool known, glm::vec2 pos, glm::vec2 size){
	if(faceCount <= 0){
		return;
	}

	faceCenterX = pos.x + size.x * 0.5f;
	detected = true;
	lookDetections++;
	if(faceCount > lookMaxFaces) lookMaxFaces = faceCount > 255 ? 255 : (uint8_t) faceCount;
	lookX = (int16_t) (faceCenterX * 100);
	lookW = (int16_t) (size.x * 100);
}

void SummonEventRoutine::onProximityReading(ProxState front, ProxState bottom){
	proxReadingReceived = true;
	const bool unsafe = (bottom == ProxState::Uncovered) || (front == ProxState::Covered);
	if(unsafe && ++unsafeReadings < StartCheckReadings){
		retryAtMs = millis() + StartCheckRetryMs;   // one reading isn't enough: ask again
		return;
	}
	proxUnsafe = unsafe;
	startFront = front;
	startBottom = bottom;
}

void SummonEventRoutine::onProximityChange(ProxSensor sensor, bool state){
	if((sensor == ProxSensor::Bottom && !state) || (sensor == ProxSensor::Front && state)){
		if(!proxInterrupted){
			interruptCause = sensor == ProxSensor::Bottom ? "bottom sensor: floor lost" : "front sensor: something close";
		}
		proxInterrupted = true;
	}
}

void SummonEventRoutine::onMotion(MotionType type){
	MotionService* motion = ApplicationStatics::getApplication()->getService<MotionService>();
	if(motion != nullptr && !motion->isLifted()) return;   // stale: the floor is back
	if(!proxInterrupted){
		static constexpr const char* Names[] = { "motion: fall", "motion: upside down", "motion: pickup", "motion: shake" }; // MotionType order
		const uint8_t i = static_cast<uint8_t>(type);
		interruptCause = i < 4 ? Names[i] : "motion";
	}
	// Motion events only fire while lifted off the ground; treat any as an unsafe interruption.
	proxInterrupted = true;
}

void SummonEventRoutine::note(const char* fmt, ...) const{
	VoiceLink* link = ApplicationStatics::getApplication()->getService<VoiceLink>();
	if(link == nullptr) return;
	char text[128];
	va_list args;
	va_start(args, fmt);
	vsnprintf(text, sizeof(text), fmt, args);
	va_end(args);
	char msg[176];
	snprintf(msg, sizeof(msg), "{\"type\":\"log\",\"text\":\"%s\"}", text);
	link->sendText(msg);
}

void SummonEventRoutine::beginLook(){
	lookStartFrames = faceDet != nullptr ? faceDet->frameCount() : 0;
	lookDetections = 0;
	lookMaxFaces = 0;
	lookX = -1;
	lookW = 0;
}

void SummonEventRoutine::reportLook() const{
	const uint32_t frames = faceDet != nullptr ? faceDet->frameCount() - lookStartFrames : 0;
	note("find me: look %u of %u: %lu frames, %u detections, max %u faces", rotationCount + 1, MaxRotations + 1,
		 (unsigned long) frames, lookDetections, lookMaxFaces);
}
