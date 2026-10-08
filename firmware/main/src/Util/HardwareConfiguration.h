#ifndef BUTTERBOT_FIRMWARE_HARDWARECONFIGURATION_H
#define BUTTERBOT_FIRMWARE_HARDWARECONFIGURATION_H

#include <array>
#include <vector>
#include <cstdint>
#include "Drivers/Interface/OutputDriver.h"
#include "Drivers/Interface/InputDriver.h"
#include "Drivers/Input/InputGPIO.h"
#include "Drivers/Output/OutputPWM.h"
#include "Enums.h"
#include <driver/i2s_std.h>
#include <driver/i2s_pdm.h>
#include <esp_camera.h>
#include "Pins.hpp"

class HardwareConfiguration {
public:
	HardwareConfiguration() = delete;

	static constexpr uint8_t getTcaAddress()       { return TcaAddress; }
	static constexpr uint8_t getBaseBoardAddress() { return BaseBoardAddress; }
	static constexpr uint8_t getAcceleroAddress() { return AcceleroAddress; }
	static constexpr uint8_t getRTCAddress() { return RTCAddress; }

	static constexpr camera_config_t getCameraConfig(int i2cPort) {
		camera_config_t cfg = CameraConfig;
		cfg.sccb_i2c_port = i2cPort;
		return cfg;
	}

	static constexpr const i2s_pdm_rx_config_t& getMicI2SConfig()   { return MicI2SConfig; }
	static constexpr const i2s_std_config_t&    getAudioI2SConfig() { return AudioI2SConfig; }

	// std::vector cannot be static constexpr (C++23 disallows persistent constexpr allocation),
	// so these getters wrap the constexpr arrays in a one-time-initialized static vector
	// because the driver constructors require const std::vector<...>&.
	static const std::vector<OutputPinDef>& getTcaOutputPins() {
		static const std::vector<OutputPinDef> v(TcaOutputPins.begin(), TcaOutputPins.end());
		return v;
	}

	static const std::vector<InputPinDef>& getTcaInputPins() {
		static const std::vector<InputPinDef> v(TcaInputPins.begin(), TcaInputPins.end());
		return v;
	}

	static const std::vector<OutputPinDef>& getGpioOutputPins() {
		static const std::vector<OutputPinDef> v(GpioOutputPins.begin(), GpioOutputPins.end());
		return v;
	}

	static const std::vector<GPIOPinDef>& getGpioInputPins() {
		static const std::vector<GPIOPinDef> v(GpioInputPins.begin(), GpioInputPins.end());
		return v;
	}

	static const std::vector<OutputPWMPinDef>& getPwmOutputPins() {
		static const std::vector<OutputPWMPinDef> v(PwmOutputPins.begin(), PwmOutputPins.end());
		return v;
	}

private:
	static constexpr uint8_t TcaAddress       = 0x20;
	static constexpr uint8_t BaseBoardAddress = 0x69;
	static constexpr uint8_t AcceleroAddress = 0x18;
	static constexpr uint8_t RTCAddress = 0x51;

	static constexpr std::array<OutputPinDef, 7> TcaOutputPins = {{
		{ .port = EXP_PIN_LED },
		{ .port = EXP_CALIB_EN },
		{ .port = EXP_SD_MODE_PIN },
		{ .port = EXP_CTRL_3 },
		{ .port = EXP_CTRL_4 },
		{ .port = EXP_CTRL_5 },
		{ .port = 8 },
	}};

	static constexpr std::array<InputPinDef, 8> TcaInputPins = {{
		{ .port = EXP_DET_1 },
		{ .port = EXP_DET_2 },
		{ .port = EXP_ADDR_1 },
		{ .port = EXP_ADDR_2 },
		{ .port = EXP_ADDR_3 },
		{ .port = EXP_ADDR_4 },
		{ .port = EXP_ADDR_5 },
		{ .port = EXP_ADDR_6 },
	}};

	static constexpr std::array<OutputPinDef, 3> GpioOutputPins = {{
		{ .port = CTRL_1 },
		{ .port = CTRL_2, .inverted = true },
		{ .port = CTRL_6 },
	}};

	static constexpr std::array<OutputPWMPinDef, 1> PwmOutputPins = {{
		{ { static_cast<int>(PWMChannel::ModuleLED), true }, (gpio_num_t)CTRL_1 },
	}};

	static constexpr std::array<GPIOPinDef, 4> GpioInputPins = {{
		GPIOPinDef{{ CTRL_1, false }, PullMode::None },
		GPIOPinDef{{ CTRL_2, false }, PullMode::None },
		GPIOPinDef{{ CTRL_6, false }, PullMode::Up },
		GPIOPinDef{{ PIN_BTN, false }, PullMode::None },
	}};

	static constexpr camera_config_t CameraConfig = {
		.pin_pwdn      = CAM_PIN_PWDN,
		.pin_reset     = CAM_PIN_RESET,
		.pin_xclk      = CAM_PIN_XCLK,
		.pin_sccb_sda  = -1,
		.pin_sccb_scl  = -1,
		.pin_d7        = CAM_PIN_D7,
		.pin_d6        = CAM_PIN_D6,
		.pin_d5        = CAM_PIN_D5,
		.pin_d4        = CAM_PIN_D4,
		.pin_d3        = CAM_PIN_D3,
		.pin_d2        = CAM_PIN_D2,
		.pin_d1        = CAM_PIN_D1,
		.pin_d0        = CAM_PIN_D0,
		.pin_vsync     = CAM_PIN_VSYNC,
		.pin_href      = CAM_PIN_HREF,
		.pin_pclk      = CAM_PIN_PCLK,
		.xclk_freq_hz  = 27000000,
		.ledc_timer    = LEDC_TIMER_0,
		.ledc_channel  = LEDC_CHANNEL_0,
		.fb_count      = 1,
		.fb_location   = CAMERA_FB_IN_PSRAM,
		.grab_mode     = CAMERA_GRAB_LATEST,
		.sccb_i2c_port = 0,
	};

	static constexpr i2s_pdm_rx_config_t MicI2SConfig = {
		.clk_cfg  = I2S_PDM_RX_CLK_DEFAULT_CONFIG(16000),
		.slot_cfg = I2S_PDM_RX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
		.gpio_cfg = {
			.clk = static_cast<gpio_num_t>(I2S_CLK_MIC),
			.din = static_cast<gpio_num_t>(I2S_DATA_IN_PIN),
			.invert_flags = {
				.clk_inv = false,
			},
		},
	};

	static constexpr i2s_std_config_t AudioI2SConfig = {
		.clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
		.slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
		.gpio_cfg = {
			.mclk = I2S_GPIO_UNUSED,
			.bclk = static_cast<gpio_num_t>(I2S_BCK_PIN),
			.ws   = static_cast<gpio_num_t>(I2S_WS_PIN),
			.dout = static_cast<gpio_num_t>(I2S_DATA_OUT_PIN),
			.din  = I2S_GPIO_UNUSED,
			.invert_flags = {
				.mclk_inv = false,
				.bclk_inv = false,
				.ws_inv   = false,
			},
		},
	};
};

#endif //BUTTERBOT_FIRMWARE_HARDWARECONFIGURATION_H
