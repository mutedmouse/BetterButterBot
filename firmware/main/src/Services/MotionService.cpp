#include "MotionService.h"
#include <Core/Application.h>
#include <Util/ServiceLocator.h>

DEFINE_LOG(MotionService)

MotionService::MotionService(){
	sem = xSemaphoreCreateBinary();

	fifo.reserve(MaxFIFODepth);

	auto baseboard = getApp()->getService<BaseBoard>();
	if(baseboard == nullptr){
		CMF_LOG(MotionService, LogLevel::Error, "BaseBoard service missing in constructor!");
		return;
	}

	thread = std::make_unique<Threaded>([this]() { loop(); }, "MotionService", 0, 2 * 1024, CONFIG_CMF_THREADED_PRIORITY, CONFIG_CMF_THREADED_CPU_CORE, false);
}

MotionService::~MotionService(){
	if(auto baseboard = getApp()->getService<BaseBoard>()){
		baseboard->setProximityScanning(false);
		baseboard->onProximityChange.unbind(this);
		baseboard->onProximityReading.unbind(this);
	}

	proximityLifted = false;
	processingEnabled = false;
	thread->stop(0);
	xSemaphoreGive(sem);
	thread->stop();
	vSemaphoreDelete(sem);
}

void MotionService::postInitProperties() noexcept{
	// Proximity tracking is live for the whole lifetime of the service, independent of enable()/disable().
	auto baseboard = getApp()->getService<BaseBoard>();
	if(baseboard == nullptr){
		CMF_LOG(MotionService, LogLevel::Error, "BaseBoard service missing in postInitProperties!");
	} else{
		baseboard->setProximityScanning(true);
		baseboard->onProximityChange.bind(this, &MotionService::proximityChanged);
		baseboard->onProximityReading.bind(this, &MotionService::proximityReading);
		baseboard->requestProximityState();
	}

	thread->start();
}

void MotionService::enable(){
	// Only gates motion processing; proximity tracking is set up in postInitProperties and stays live.
	processingEnabled = true;

	// If already lifted, wake the loop so it starts processing immediately.
	if(proximityLifted){
		xSemaphoreGive(sem);
	}
}

void MotionService::disable(){
	// The loop observes this, breaks out, stops the FIFO and parks itself on the semaphore.
	processingEnabled = false;
}

bool MotionService::isUprightOnGround(){
	// Bottom proximity is the ground reference; if ButterBot has been lifted, it isn't on the ground.
	// While on the ground the sampling loop is parked on the semaphore, so this read can't race readFIFO().
	if(proximityLifted){
		return false;
	}

	// Average several samples: the robot still bounces from its last move, and its own speaker ("On my
	// way" just before) shakes it, so single samples read as "moving" and refused good starts.
	SC7A20::Sample avg{};
	for(int i = 0; i < UprightSamples; i++){
		const SC7A20::Sample s = ServiceLocator::SC7A20Instance->getSample();
		avg.accelX += s.accelX / UprightSamples;
		avg.accelY += s.accelY / UprightSamples;
		avg.accelZ += s.accelZ / UprightSamples;
		if(i + 1 < UprightSamples) delayMillis(UprightSampleGapMs);
	}
	lastUpright = avg;
	return detectPose(avg) == Pose::Straight;
}

void MotionService::proximityReading(ProxState front, ProxState bottom){
	if(bottom == ProxState::Unknown) return;
	const bool first = !initialReadingDone.exchange(true);

	if(bottom == ProxState::Covered){
		proximityLifted = false;   // floor seen: clears at once
		return;
	}
	// Floor not seen. Only the startup reading may mark the robot lifted: routines request one-off
	// readings before moving, and on a dark floor one can read "no floor" for an instant. Latching that
	// left the robot "lifted" (refusing to move, reading its own motion as shakes) until the motor board
	// happened to report a change, sometimes minutes later. Real lifts arrive as debounced changes.
	if(first){
		proximityLifted = true;
		CMF_LOG(MotionService, LogLevel::Debug, "initial reading: bottom IR raised");
		xSemaphoreGive(sem);
	}
}

void MotionService::proximityChanged(ProxSensor sensor, bool thresholdChange){
	if(sensor != ProxSensor::Bottom) return;

	if(!thresholdChange){
		CMF_LOG(MotionService, LogLevel::Debug, "bottom IR raised");
		proximityLifted = true;
		xSemaphoreGive(sem);
	} else{
		CMF_LOG(MotionService, LogLevel::Debug, "bottom IR dropped");
		proximityLifted = false;
	}
}

void MotionService::loop() noexcept{
	xSemaphoreTake(sem, portMAX_DELAY);

	// Spurious wake (e.g. lifted while disabled, or disabled before we ran): re-park on next invocation.
	if(!(proximityLifted && processingEnabled)) return;

	// All SC7A20 FIFO access stays on this thread, so start/stop the FIFO here rather than in enable()/disable().
	ServiceLocator::SC7A20Instance->startFIFO();

	shake.reset();
	lastPose = Pose::Moving;
	lastEvent.reset();
	candidateStart = millis();
	candidate = MotionType::Shake;
	pickupDone = false;

	while(proximityLifted && processingEnabled){
		delayMillis(FIFOFillTime);

		//check after delayMillis
		if(!(proximityLifted && processingEnabled)) break;

		fifo.clear();

		if(ServiceLocator::SC7A20Instance){
			ServiceLocator::SC7A20Instance->readFIFO(fifo);
		}
		if(fifo.empty()) continue;

		// for(uint8_t i = 0; i < fifo.size(); i++){
		// 	printf("x: %.2f, y: %.2f, z: %.2f \n", fifo[i].accelX, fifo[i].accelY, fifo[i].accelZ);
		// }

		Pose currentPose = detectPose(fifo[0]);

		for(const auto& sample : fifo){
			shake.onAdd(sample);
		}

		if(shake.detected()){
			lastPose = Pose::Moving;
			candidate = MotionType::Shake;
			lastEvent = MotionType::Shake;
			candidateStart = millis();
			onMotion.broadcast(MotionType::Shake);
		}

		MotionType currentCandidate = MotionType::Shake;

		if(currentPose == Pose::Moving || currentPose == Pose::Straight){
			//Pickup
			currentCandidate = MotionType::Pickup;
		} else if(currentPose == Pose::UpsideDown){
			//upside-down
			currentCandidate = MotionType::UpsideDown;
		} else{
			currentCandidate = MotionType::Fall;
		}

		if(currentCandidate != candidate ||
			(currentCandidate == candidate && candidate == MotionType::Fall && lastPose != currentPose)){
			//New candidate appeared. Specifically, "Fall" is reset if pose changes
			candidateStart = millis();
			lastPose = currentPose;
			candidate = currentCandidate;
		} else if(millis() - candidateStart >= CandidateHoldTime){
			//Candidate stayed the same, check for hold time
			//Do not re-broadcast same event
			if(lastEvent.has_value() && lastEvent.value() == candidate){
				continue;
			}

			//Do not repeat pickup
			if(candidate == MotionType::Pickup && pickupDone){
				continue;
			}

			//Mark pickup as done
			if(candidate == MotionType::Pickup && !pickupDone){
				pickupDone = true;
			}

			if(candidate == MotionType::Fall || candidate == MotionType::UpsideDown){
				//Fall or upside-down reset occurence of pickup, while shake does not
				pickupDone = false;
			}

			onMotion.broadcast(candidate);
			lastEvent = candidate;
		}
	}

	ServiceLocator::SC7A20Instance->stopFIFO();
}

MotionService::Pose MotionService::detectPose(SC7A20::Sample sample){
	static constexpr float MinMag2 = 0.7f * 0.7f;
	static constexpr float MaxMag2 = 1.3f * 1.3f;

	const float mag2 = sample.accelX * sample.accelX + sample.accelY * sample.accelY + sample.accelZ * sample.accelZ;
	if(mag2 < MinMag2 || mag2 > MaxMag2) return Pose::Moving;


	//Check for dominant axis, then determine pose
	if(abs(sample.accelX) > abs(sample.accelY) && abs(sample.accelX) > abs(sample.accelZ)){
		if(sample.accelX > AxisMin){
			return Pose::Straight;
		}
		if(sample.accelX < AxisMax){
			return Pose::UpsideDown;
		}
	} else if(abs(sample.accelY) > abs(sample.accelX) && abs(sample.accelY) > abs(sample.accelZ)){
		if(sample.accelY > AxisMin){
			return Pose::Left;
		}
		if(sample.accelY < AxisMax){
			return Pose::Right;
		}
	} else if(abs(sample.accelZ) > abs(sample.accelX) && abs(sample.accelZ) > abs(sample.accelY)){
		if(sample.accelZ > AxisMin){
			return Pose::Backward;
		}
		if(sample.accelZ < AxisMax){
			return Pose::Forward;
		}
	}

	return Pose::Moving;
}

bool MotionService::ShakeDetector::detected(){
	if(count == 0) return false;

	const float n = (float)count;
	const float varX = sumX2 / n - (sumX / n) * (sumX / n);
	const float varY = sumY2 / n - (sumY / n) * (sumY / n);
	const float varZ = sumZ2 / n - (sumZ / n) * (sumZ / n);

	CMF_LOG(MotionService, LogLevel::Debug, "Shake: varX=%.3f varY=%.3f varZ=%.3f (thresh=%.3f)", varX, varY, varZ, VarianceThreshold);
	const bool highVariance = varX > VarianceThreshold || varY > VarianceThreshold || varZ > VarianceThreshold;
	if(highVariance && !isShaking){
		isShaking = true;
		return true;
	}
	if(!highVariance && isShaking) isShaking = false;
	return false;
}
