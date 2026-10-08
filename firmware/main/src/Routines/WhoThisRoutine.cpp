#include "WhoThisRoutine.h"
#include <Statics/ApplicationStatics.h>
#include <Core/Application.h>
#include <BBData.h>
#include "FaceDet.h"
#include "Services/Com.h"

DEFINE_LOG(WhoThisRoutine)

bool WhoThisRoutine::prepare(FaceDet* faceDet){
	return true;
}

void WhoThisRoutine::onScanStart(){
	const int16_t id = Phrases::get(Phrase::FaceScanStart);
	if(id < 0) return;

	if(Com* com = ApplicationStatics::getApplication()->getService<Com>()){
		FaceDetectData data{ .state = FaceDetectData::State::Scanning };
		com->sendData(BB::State::Scenario, BB::Action::Scenario::FaceDetect, data);
	}

	speak(Phrase::FaceScanStart, id);
}

void WhoThisRoutine::onFaceDetected(FaceDet* faceDet, bool known){
	const Phrase phrase = known ? Phrase::FaceOwnerRecognized : Phrase::FaceStrangerRecognized;
	const int16_t id = Phrases::get(phrase);
	if(id < 0) return;

	if(Com* com = ApplicationStatics::getApplication()->getService<Com>()){
		FaceDetectData data{ .state = known ? FaceDetectData::State::Recognized : FaceDetectData::State::NotRecognized };
		com->sendData(BB::State::Scenario, BB::Action::Scenario::FaceDetect, data);
	}

	speak(phrase, id);
}

void WhoThisRoutine::onScanTimeout(){
	const int16_t id = Phrases::get(Phrase::FaceNotDetected);
	if(id < 0) return;

	if(Com* com = ApplicationStatics::getApplication()->getService<Com>()){
		FaceDetectData data{ .state = FaceDetectData::State::NoFaceFound };
		com->sendData(BB::State::Scenario, BB::Action::Scenario::FaceDetect, data);
	}

	speak(Phrase::FaceNotDetected, id);
}
