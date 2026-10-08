#include "LIS2DW12.h"
#include "Log/Log.h"

DEFINE_LOG(LIS2DW12)

LIS2DW12::LIS2DW12(std::unique_ptr<I2CDevice> i2cDevice, PinConfig pinConfig, Config config, uint8_t addr) : i2c(std::move(i2cDevice)), Addr(addr), config(config), pins(pinConfig){
	dispatcherThread = std::make_unique<Threaded>([this](){ dispatcherFunc(); }, "LIS2_dispatcher", 0, 2 * 1024, 8, CONFIG_CMF_THREADED_CPU_CORE, false);

	if(!i2c){
		CMF_LOG(LIS2DW12, LogLevel::Error, "No I2C peripheral provided");
		abort();
	}

	dispatcherSem = xSemaphoreCreateBinary();

	init(config, pins);

	dispatcherThread->start();
}

LIS2DW12::~LIS2DW12(){
	dispatcherThread->stop();
}

LIS2DW12::Sample LIS2DW12::getSample(){
	int16_t accel[3];
	lis2dw12_acceleration_raw_get(&ctx, accel);

	std::function<float(uint16_t)> conversionFunc;
	switch(config.fullScale){
		case LIS2DW12_2g:
			conversionFunc = lis2dw12_from_fs2_to_mg;
			break;
		case LIS2DW12_4g:
			conversionFunc = lis2dw12_from_fs4_to_mg;
			break;
		case LIS2DW12_8g:
			conversionFunc = lis2dw12_from_fs8_to_mg;
			break;
		case LIS2DW12_16g:
			conversionFunc = lis2dw12_from_fs16_to_mg;
			break;
	}

	Sample sample = {
			conversionFunc(accel[0]) / 1000.0f,
			conversionFunc(accel[1]) / 1000.0f,
			conversionFunc(accel[2]) / 1000.0f
	};

	return sample;
}

LIS2DW12::Sample LIS2DW12::pollFIFO(TickType_t timeout) const {
	if(!queue){
		CMF_LOG(LIS2DW12, LogLevel::Error, "FIFO not enabled");
		return Sample{};
	}

	Sample sample{};
	if(xQueueReceive(queue, &sample, timeout) != pdTRUE){
		CMF_LOG(LIS2DW12, LogLevel::Warning, "Timeout waiting for FIFO sample");
		return Sample{};
	}
	return sample;
}

void LIS2DW12::init(Config config, PinConfig pins){
	lis2dw12_reset_set(&ctx);
	uint8_t resetVal;
	do {
		lis2dw12_reset_get(&ctx, &resetVal);
	} while(resetVal);

	uint8_t id;
	lis2dw12_device_id_get(&ctx, &id);
	if(id != LIS2DW12_ID){
		CMF_LOG(LIS2DW12, LogLevel::Error, "Init error, got ID 0x%x, expected 0x%x", id, LIS2DW12_ID);
		abort();
	}

	lis2dw12_block_data_update_set(&ctx, 1); //for reading 2 byte long values
	clearSources();

	lis2dw12_power_mode_set(&ctx, config.powerMode);
	lis2dw12_data_rate_set(&ctx, config.sampleRate);
	lis2dw12_full_scale_set(&ctx, config.fullScale);

	lis2dw12_filter_path_set(&ctx, config.filterType);
	lis2dw12_filter_bandwidth_set(&ctx, config.filterBandwidth);

	lis2dw12_int_notification_set(&ctx, LIS2DW12_INT_LATCHED);
	lis2dw12_pin_int1_route_set(&ctx, &pins.int1_routing);
	lis2dw12_pin_int2_route_set(&ctx, &pins.int2_routing);

	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_HIGH_LEVEL;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	gpio_install_isr_service(ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM);


	if(pins.int1 != GPIO_NUM_NC){
		io_conf.pin_bit_mask = (1ULL << pins.int1);
		gpio_config(&io_conf);
		gpio_isr_handler_add(pins.int1, isr, this);
	}

	if(pins.int2 != GPIO_NUM_NC){
		io_conf.pin_bit_mask = (1ULL << pins.int2);
		gpio_config(&io_conf);
		gpio_isr_handler_add(pins.int2, isr, this);
	}
}

int32_t LIS2DW12::platform_write(void* hndl, uint8_t reg, const uint8_t* data, uint16_t len){
	auto accel = static_cast<LIS2DW12*>(hndl);
	return accel->i2c->writeRegister(reg, data, len);
}

int32_t LIS2DW12::platform_read(void* hndl, uint8_t reg, uint8_t* data, uint16_t len){
	auto accel = static_cast<LIS2DW12*>(hndl);
	return accel->i2c->write_read(&reg, 1, data, len);
}

void LIS2DW12::enableFreeFallDetection(LIS2DW12::FreeFallConfig FFConfig) const {
	lis2dw12_ff_threshold_set(&ctx, FFConfig.threshold);
	lis2dw12_ff_dur_set(&ctx, FFConfig.duration);
}

void LIS2DW12::disableFreeFallDetection() const {
	lis2dw12_ff_dur_set(&ctx, 0);
}

void LIS2DW12::enableTapDetection(LIS2DW12::TapConfig tapConfig) const {
	lis2dw12_tap_mode_set(&ctx, tapConfig.mode);
	lis2dw12_tap_threshold_x_set(&ctx, tapConfig.thresholdX);
	lis2dw12_tap_threshold_y_set(&ctx, tapConfig.thresholdY);
	lis2dw12_tap_threshold_z_set(&ctx, tapConfig.thresholdZ);

//	lis2dw12_tap_axis_priority_set(&ctx, tapConfig.axisPriority);
	lis2dw12_tap_shock_set(&ctx, tapConfig.shock);
	lis2dw12_tap_quiet_set(&ctx, tapConfig.quiet);
	lis2dw12_tap_dur_set(&ctx, tapConfig.doubleTapDuration);

	lis2dw12_tap_detection_on_x_set(&ctx, tapConfig.enableX);
	lis2dw12_tap_detection_on_y_set(&ctx, tapConfig.enableY);
	lis2dw12_tap_detection_on_z_set(&ctx, tapConfig.enableZ);
}

void LIS2DW12::disableTapDetection() const {
	lis2dw12_tap_detection_on_x_set(&ctx, 0);
	lis2dw12_tap_detection_on_y_set(&ctx, 0);
	lis2dw12_tap_detection_on_z_set(&ctx, 0);
}

void LIS2DW12::enableFIFO(LIS2DW12::FIFOConfig fifoConfig){
	queue = xQueueCreate(FIFOMaxSamples, sizeof(Sample));
	lis2dw12_fifo_mode_set(&ctx, fifoConfig.mode);
	lis2dw12_fifo_watermark_set(&ctx, fifoConfig.threshold);
}

void LIS2DW12::disableFIFO(){
	lis2dw12_fifo_mode_set(&ctx, LIS2DW12_BYPASS_MODE);
	if(queue){
		vQueueDelete(queue);
		queue = nullptr;
	}
}

void LIS2DW12::clearSources() const {
	lis2dw12_all_sources_t src;
	lis2dw12_all_sources_get(&ctx, &src);
}

void IRAM_ATTR LIS2DW12::isr(void* arg){
	auto lis2 = static_cast<LIS2DW12*>(arg);

	if(lis2->pins.int1 != GPIO_NUM_NC){
		gpio_intr_disable(lis2->pins.int1);
	}

	if(lis2->pins.int2 != GPIO_NUM_NC){
		gpio_intr_disable(lis2->pins.int2);
	}

	auto wake = pdTRUE;
	xSemaphoreGiveFromISR(lis2->dispatcherSem, &wake);
}

void LIS2DW12::dispatcherFunc(){
	if(xSemaphoreTake(dispatcherSem, portMAX_DELAY) != pdTRUE){
		CMF_LOG(LIS2DW12, LogLevel::Warning, "Failed taking dispatcher semaphore");
		return;
	}

	CMF_LOG(LIS2DW12, LogLevel::Verbose, "dispatcher unblocked");

	while(gpio_get_level(pins.int1) || gpio_get_level(pins.int2)){
		fetchEvents();
	}

	if(pins.int1 != GPIO_NUM_NC){
		gpio_intr_enable(pins.int1);
	}
	if(pins.int2 != GPIO_NUM_NC){
		gpio_intr_enable(pins.int2);
	}

}

void LIS2DW12::fetchEvents(){
	lis2dw12_all_sources_t src;
	lis2dw12_all_sources_get(&ctx, &src);

	if(src.tap_src.tap_ia){
		event.broadcast(EventType::Tap);
	}

	if(src.all_int_src.ff_ia){
		event.broadcast(EventType::FreeFall);
	}


	//FIFO queue initialized?
	if(queue){

		uint8_t fifoThresh = 0;
		lis2dw12_fifo_wtm_flag_get(&ctx, &fifoThresh);

		if(fifoThresh){
			event.broadcast(EventType::FIFOOverflow);
		}

		uint8_t numReadings;
		lis2dw12_fifo_data_level_get(&ctx, &numReadings);

		for(int i = 0; i < numReadings; ++i){
			int16_t accel[3];
			lis2dw12_acceleration_raw_get(&ctx, accel);

			std::function<float(uint16_t)> conversionFunc;
			switch(config.fullScale){
				case LIS2DW12_2g:
					conversionFunc = lis2dw12_from_fs2_to_mg;
					break;
				case LIS2DW12_4g:
					conversionFunc = lis2dw12_from_fs4_to_mg;
					break;
				case LIS2DW12_8g:
					conversionFunc = lis2dw12_from_fs8_to_mg;
					break;
				case LIS2DW12_16g:
					conversionFunc = lis2dw12_from_fs16_to_mg;
					break;
			}

			Sample sample = {
					conversionFunc(accel[0]) / 1000.0f,
					conversionFunc(accel[1]) / 1000.0f,
					conversionFunc(accel[2]) / 1000.0f
			};
			xQueueSend(queue, &sample, 0);
		}
	}
}
