#ifndef BUTTERBOT_FIRMWARE_OBJDET_H
#define BUTTERBOT_FIRMWARE_OBJDET_H

#include "Devices/Camera.h"
#include <cstddef>
#include <vector>
#include <cstdint>
#include <esp_partition.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include <tensorflow/lite/micro/micro_mutable_op_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <ObjDetClass.h>


class ObjDet {
public:
	ObjDet();
	virtual ~ObjDet();

	static const std::unordered_map<ObjClass, const char*> ClassNames;

	void init();
	void deinit();

	void loadModel();
	void unloadModel();

	std::vector<ObjClass> detect();

private:
	bool inited = false;
	Camera* cam = nullptr;

	static constexpr size_t PoolSize = 1024 * 1024;
	std::vector<uint8_t> tensorPool;

	const void* modelData = nullptr;
	esp_partition_mmap_handle_t dataHndl = 0;

	const tflite::Model* model = nullptr;
	tflite::MicroMutableOpResolver<14> resolver;
	std::unique_ptr<tflite::MicroInterpreter> interpreter;

	struct Result {
		ObjClass cls;
		float score;
	};

	std::vector<Result> once();

	bool getFrame(void* dst);

};


#endif //BUTTERBOT_FIRMWARE_OBJDET_H
