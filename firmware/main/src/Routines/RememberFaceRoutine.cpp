#include "RememberFaceRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <BBData.h>
#include "FaceDet.h"
#include "Services/Com.h"

DEFINE_LOG(RememberFaceRoutine)

bool RememberFaceRoutine::prepare(FaceDet* faceDet){
	if(!faceDet->knowsFace()){
		return true;
	}

	const int16_t id = Phrases::get(Phrase::FaceAlreadyOwner);
	if(id < 0) return false;

	if(Com* com = ApplicationStatics::getApplication()->getService<Com>()){
		FaceDetectData data{ .state = FaceDetectData::State::AlreadySaved };
		com->sendData(BB::State::Scenario, BB::Action::Scenario::FaceDetect, data);
	}

	speak(Phrase::FaceAlreadyOwner, id);
	return false;
}

void RememberFaceRoutine::onScanStart(){
	const int16_t id = Phrases::get(Phrase::FaceScanStart);
	if(id < 0) return;

	if(Com* com = ApplicationStatics::getApplication()->getService<Com>()){
		FaceDetectData data{.state = FaceDetectData::State::Scanning};
		com->sendData(BB::State::Scenario, BB::Action::Scenario::FaceDetect, data);
	}

	speak(Phrase::FaceScanStart, id);
}

void RememberFaceRoutine::startScan(FaceDet* faceDet){
	faceDet->learnFace();
}

void RememberFaceRoutine::onFaceDetected(FaceDet* faceDet, bool known){
	const int16_t id = Phrases::get(Phrase::FaceRegistered);
	if(id < 0) return;

	if(Com* com = ApplicationStatics::getApplication()->getService<Com>()){
		FaceDetectData data{.state = FaceDetectData::State::NewOwner};
		com->sendData(BB::State::Scenario, BB::Action::Scenario::FaceDetect, data);
	}

	speak(Phrase::FaceRegistered, id);
}

void RememberFaceRoutine::onScanTimeout(){
	const int16_t id = Phrases::get(Phrase::FaceNotDetected);
	if(id < 0) return;

	if(Com* com = ApplicationStatics::getApplication()->getService<Com>()){
		FaceDetectData data{.state = FaceDetectData::State::NoFaceFound};
		com->sendData(BB::State::Scenario, BB::Action::Scenario::FaceDetect, data);
	}

	speak(Phrase::FaceNotDetected, id);
}
