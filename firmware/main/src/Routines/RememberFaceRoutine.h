#ifndef BUTTERBOT_FIRMWARE_REMEMBERFACEROUTINE_H
#define BUTTERBOT_FIRMWARE_REMEMBERFACEROUTINE_H

#include "FaceRoutine.h"

class RememberFaceRoutine : public FaceRoutine {
	public:
	using FaceRoutine::FaceRoutine;

protected:
	virtual bool prepare(FaceDet* faceDet) override;
	virtual void onScanStart() override;
	virtual void startScan(FaceDet* faceDet) override;
	virtual void onFaceDetected(FaceDet* faceDet, bool known) override;
	virtual void onScanTimeout() override;
};

#endif //BUTTERBOT_FIRMWARE_REMEMBERFACEROUTINE_H
