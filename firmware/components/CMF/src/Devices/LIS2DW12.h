#ifndef CMF_LIS2DW12_H
#define CMF_LIS2DW12_H

#include "Object/Class.h"
#include "Object/Object.h"
#include <driver/gpio.h>
#include "lis2dw12_reg.h"
#include "Periphery/I2CDevice.h"
#include "Event/EventBroadcaster.h"

//TODO - factor out an accelerometer interface, and a FIFO-capable accelerometer abstract class

/**
 * 3-axis accelerometer (https://www.st.com/resource/en/datasheet/lis2dw12.pdf)
 */
class LIS2DW12 : public Object {
public:
	struct FreeFallConfig {
		lis2dw12_ff_ths_t threshold;
		/**
		 * duration: 1 LSB = 1 * 1/ODR
		 * (for example if value is 50, and ODR is 100Hz, the actual duration threshold is 50/(100 Hz) = 0.5s
		 */
		uint8_t duration: 6;
	};

	struct TapConfig {
		lis2dw12_single_double_tap_t mode; //single or single-double
		uint8_t thresholdX: 5;
		uint8_t thresholdY: 5;
		uint8_t thresholdZ: 5;
		lis2dw12_tap_prior_t axisPriority;
		bool enableX: 1;
		bool enableY: 1;
		bool enableZ: 1;
		/**
		 * Maximum duration is the maximum time of an overthreshold signal detection to be recognized as a tap event.
		 * The default value of these bits is 00b which corresponds to 4*ODR_XL time.
		 * If the SHOCK[1:0] bits are set to a different value, 1LSB corresponds to 8*ODR_XL time.
		 */
		uint8_t shock: 2;
		/**
		 * Quiet time is the time after the first detected tap in which there must not be any overthreshold event.
		 * The default value of these bits is 00b which corresponds to 2*ODR_XL time.
		 * If the QUIET[1:0] bits are set to a different value, 1LSB corresponds to 4*ODR_XL time.
		 */
		uint8_t quiet: 2;

		/**
		 * When double tap recognition is enabled, this register expresses the maximum time between two consecutive detected taps to determine a double tap event.
		 * The default value of these bits is 0000b which correspondsto 16*ODR_XL time.
		 * If the DUR[3:0] bits are set to a different value, 1LSB corresponds to 32*ODR_XL time.
		 */
		uint8_t doubleTapDuration: 4;
	};

	struct FIFOConfig {
		lis2dw12_fmode_t mode;
		uint8_t threshold: 5; //after this many samples are read, FIFO flag is set and also optional interrupt raised
	};


	struct Config {
		lis2dw12_mode_t powerMode;
		lis2dw12_odr_t sampleRate;
		lis2dw12_fs_t fullScale;
		lis2dw12_fds_t filterType;
		lis2dw12_bw_filt_t filterBandwidth; //valid for low/hi pass filter
	};

	struct PinConfig {
		gpio_num_t int1;
		lis2dw12_ctrl4_int1_pad_ctrl_t int1_routing;
		gpio_num_t int2;
		lis2dw12_ctrl5_int2_pad_ctrl_t int2_routing;
	};

	struct Sample {
		float accelX;
		float accelY;
		float accelZ;
	};

	GENERATED_BODY(LIS2DW12, Object, CONSTRUCTOR_PACK(std::unique_ptr<I2CDevice>, LIS2DW12::PinConfig, LIS2DW12::Config, uint8_t))

public:
	explicit LIS2DW12(std::unique_ptr<I2CDevice> i2cDevice = {}, PinConfig pinConfig = {}, Config config = {}, uint8_t addr = (LIS2DW12_I2C_ADD_L >> 1));

	virtual ~LIS2DW12() override;

	Sample getSample();

	/**
	 * Take one sample out of the FIFO queue.
	 * Blocking function.
	 * @param timeout Timeout when taking from an empty FIFO queue
	 * @return sample
	 */
	Sample pollFIFO(TickType_t timeout = portMAX_DELAY) const;

	enum class EventType {
		FreeFall, Tap, FIFOOverflow
	};


	DECLARE_EVENT(AcceleroEvent, LIS2DW12, EventType);
	AcceleroEvent event = AcceleroEvent{this};

	void enableFreeFallDetection(FreeFallConfig FFConfig) const;
	void disableFreeFallDetection() const;

	void enableTapDetection(TapConfig tapConfig) const;
	void disableTapDetection() const;

	void enableFIFO(FIFOConfig fifoConfig);
	void disableFIFO();

private:
	std::unique_ptr<I2CDevice> i2c;
	const uint8_t Addr;

	Config config;
	PinConfig pins;

	QueueHandle_t queue = nullptr;

	static constexpr uint8_t FIFOThresholdBits = 5;
	static constexpr uint8_t FIFOMaxSamples = (1 << FIFOThresholdBits) - 1;

	void init(Config config, PinConfig pins);


	static int32_t platform_write(void* hndl, uint8_t reg, const uint8_t* data, uint16_t len);
	static int32_t platform_read(void* hndl, uint8_t reg, uint8_t* data, uint16_t len);
	stmdev_ctx_t ctx = {
			.write_reg = platform_write,
			.read_reg = platform_read,
			.mdelay = vTaskDelay,
			.handle = this,
			.priv_data = nullptr
	};
	/**
	 * Read and reset all interrupt sources.
	 * This will reset all status bits and interrupt signals.
	 */
	void clearSources() const;

	static void isr(void* arg);

	SemaphoreHandle_t dispatcherSem = nullptr;

	std::unique_ptr<Threaded> dispatcherThread;
	void dispatcherFunc();
	void fetchEvents();
};


#endif //CMF_LIS2DW12_H
