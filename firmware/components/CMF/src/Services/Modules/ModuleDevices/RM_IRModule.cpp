#include "RM_IRModule.h"
#include <driver/rmt_tx.h>
#include <driver/rmt_rx.h>
#include <driver/rmt_encoder.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <Util/stdafx.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <cstring>
#include <algorithm>

static constexpr const char* TAG = "RM_IRModule";

static constexpr uint32_t RMTResolutionHz = 1000000; // 1 MHz = 1 µs per tick
static constexpr uint32_t IRCarrierHz = 38000;
static constexpr uint32_t GlitchFilterMinNs = 2500; // HW filter is 8-bit at 80 MHz — caps at ~3187 ns
static constexpr uint32_t EndOfFrameMinNs = 10000000; // 10 ms gap = end-of-frame
static constexpr uint32_t MinInterFrameGapMs = 10;
static constexpr uint32_t DefaultRepeatPeriodUs = 45000; // shortest common repeat period (SIRC)

namespace {

struct BurstEvent {
	uint8_t bufIdx;
	uint16_t numSymbols;
	int64_t timestampUs;
};

// Each IRCapture slot is armed at most once — the ISR can't overwrite a frame the task hasn't consumed.
struct BurstContext {
	IRCapture* out;
	volatile uint8_t armedIndex;
	volatile uint8_t framesArmed;
	uint8_t maxFrames;
	rmt_receive_config_t rxConfig;
	QueueHandle_t queue;
};

// ISR context. Re-arming from the task would leave only ~10 ms of margin on short-period protocols (SIRC).
bool onRecvDone(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t* data, void* ctx){
	BurstContext* burst = static_cast<BurstContext*>(ctx);
	BaseType_t woken = pdFALSE;

	BurstEvent event{};
	event.bufIdx = burst->armedIndex;
	event.numSymbols = static_cast<uint16_t>(data->num_symbols);
	event.timestampUs = esp_timer_get_time();
	xQueueSendFromISR(burst->queue, &event, &woken);

	if(burst->framesArmed < burst->maxFrames){
		burst->framesArmed = burst->framesArmed + 1;
		burst->armedIndex = burst->armedIndex + 1;
		rmt_receive(channel, burst->out->frames[burst->armedIndex].symbols, IRRawMaxSymbols * sizeof(rmt_symbol_word_t), &burst->rxConfig);
	}

	return woken == pdTRUE;
}

}

RM_IRModule::RM_IRModule(const Modules::BusPins& busPins) : Super(Modules::Type::RM_IR, busPins){
	write(false);
}

void RM_IRModule::write(bool state){
	if(pins.subAddressPins[1].outputDriver){
		pins.subAddressPins[1].outputDriver->write(pins.subAddressPins[1].outputPort, state);
	}
}

bool RM_IRModule::read(){
	if(pins.subAddressPins[0].inputDriver){
		return pins.subAddressPins[0].inputDriver->read(pins.subAddressPins[0].outputPort);
	}

	return false;
}

gpio_num_t RM_IRModule::getInputGPIO() const{
	return static_cast<gpio_num_t>(pins.subAddressPins[0].inputPort);
}

gpio_num_t RM_IRModule::getOutputGPIO() const{
	return static_cast<gpio_num_t>(pins.subAddressPins[1].outputPort);
}

bool RM_IRModule::captureBurst(IRCapture& out, uint32_t firstFrameTimeoutMs, uint32_t burstWindowMs){
	out.count = 0;

	const gpio_num_t rxPin = getInputGPIO();

	// Fully reset the pin so LEDC/PWM output is disconnected before RMT takes over.
	gpio_reset_pin(rxPin);

	auto restorePin = [this, rxPin](){
		// Reset the pin fully before reconnecting LEDC so it sees a clean state.
		gpio_reset_pin(rxPin);
		if(pins.subAddressPins[0].outputDriver){
			pins.subAddressPins[0].outputDriver->registerOutput({ pins.subAddressPins[0].outputPort });
		}
	};

	rmt_rx_channel_config_t rxChannelConfig{};
	rxChannelConfig.gpio_num = rxPin;
	rxChannelConfig.clk_src = RMT_CLK_SRC_DEFAULT;
	rxChannelConfig.resolution_hz = RMTResolutionHz;
	rxChannelConfig.mem_block_symbols = IRMaxSymbols;

	rmt_channel_handle_t rxChannel = nullptr;
	if(rmt_new_rx_channel(&rxChannelConfig, &rxChannel) != ESP_OK){
		ESP_LOGE(TAG, "Failed to create RMT RX channel");
		restorePin();
		return false;
	}

	StaticQueue_t queueControl;
	uint8_t queueStorage[IRMaxCaptureFrames * sizeof(BurstEvent)];
	QueueHandle_t queue = xQueueCreateStatic(IRMaxCaptureFrames, sizeof(BurstEvent), queueStorage, &queueControl);

	BurstContext burst{};
	burst.out = &out;
	burst.armedIndex = 0;
	burst.framesArmed = 1;
	burst.maxFrames = IRMaxCaptureFrames;
	burst.rxConfig.signal_range_min_ns = GlitchFilterMinNs;
	burst.rxConfig.signal_range_max_ns = EndOfFrameMinNs;
	burst.queue = queue;

	rmt_rx_event_callbacks_t cbs{};
	cbs.on_recv_done = onRecvDone;
	rmt_rx_register_event_callbacks(rxChannel, &cbs, &burst);
	rmt_enable(rxChannel);

	// PSRAM RX buffers are safe only because the channel is non-DMA and RMT_RX_ISR_CACHE_SAFE is off.
	if(rmt_receive(rxChannel, out.frames[0].symbols, IRRawMaxSymbols * sizeof(rmt_symbol_word_t), &burst.rxConfig) != ESP_OK){
		ESP_LOGE(TAG, "rmt_receive failed");
	} else {
		ESP_LOGI(TAG, "Listening on GPIO %d, idle level %d", rxPin, gpio_get_level(rxPin));

		// Sample the pin while waiting so a timeout can tell a dead line from an RMT problem.
		BurstEvent event{};
		bool gotFirst = false;
		int lastLevel = gpio_get_level(rxPin);
		int transitions = 0;
		const int64_t deadlineUs = esp_timer_get_time() + static_cast<int64_t>(firstFrameTimeoutMs) * 1000;
		while(esp_timer_get_time() < deadlineUs){
			if(xQueueReceive(queue, &event, 1) == pdTRUE){
				gotFirst = true;
				break;
			}
			const int level = gpio_get_level(rxPin);
			if(level != lastLevel){
				transitions++;
				lastLevel = level;
			}
		}

		if(!gotFirst){
			ESP_LOGW(TAG, "No IR frame within %lu ms (%d pin level changes sampled — %s)",
					 static_cast<unsigned long>(firstFrameTimeoutMs), transitions,
					 transitions == 0 ? "line is dead, receiver/wiring issue" : "line active, RMT capture issue");
		} else {
			const int64_t burstStartUs = event.timestampUs;

			while(true){
				const uint16_t stored = (event.numSymbols <= IRRawMaxSymbols) ? event.numSymbols : IRRawMaxSymbols;
				out.frames[event.bufIdx].count = stored;
				out.doneTimestampsUs[event.bufIdx] = event.timestampUs;
				out.count = event.bufIdx + 1;
				ESP_LOGI(TAG, "Frame %d: %d symbols", event.bufIdx, stored);

				if(out.count >= IRMaxCaptureFrames){
					break;
				}

				const int64_t elapsedMs = (esp_timer_get_time() - burstStartUs) / 1000;
				if(elapsedMs >= burstWindowMs){
					break;
				}
				const uint32_t waitMs = std::min<uint32_t>(300, burstWindowMs - elapsedMs);
				if(xQueueReceive(queue, &event, pdMS_TO_TICKS(waitMs)) != pdTRUE){
					break;
				}
			}
		}
	}

	rmt_disable(rxChannel);
	rmt_del_channel(rxChannel);
	vQueueDelete(queue);

	restorePin();

	if(out.count > 0){
		ESP_LOGI(TAG, "Captured %d frame(s), first frame: %d symbols, level0=%d",
				 out.count, out.frames[0].count, out.frames[0].symbols[0].level0);
	}

	return out.count > 0;
}

void RM_IRModule::sendIR(const rmt_symbol_word_t* frame, uint16_t frameCount, uint8_t sendCount,
						 uint32_t periodUs, uint32_t carrierHz, uint8_t carrierDutyPct){
	if(!frame || frameCount == 0){
		return;
	}

	const gpio_num_t txPin = getOutputGPIO();

	rmt_tx_channel_config_t txConfig{};
	txConfig.gpio_num = txPin;
	txConfig.clk_src = RMT_CLK_SRC_DEFAULT;
	txConfig.resolution_hz = RMTResolutionHz;
	txConfig.mem_block_symbols = RMTMemBlockSymbols;
	txConfig.trans_queue_depth = 4;

	rmt_channel_handle_t txChannel = nullptr;
	if(rmt_new_tx_channel(&txConfig, &txChannel) != ESP_OK){
		ESP_LOGE(TAG, "Failed to create RMT TX channel");
		return;
	}

	rmt_carrier_config_t carrier{};
	carrier.frequency_hz = (carrierHz >= 30000 && carrierHz <= 60000) ? carrierHz : IRCarrierHz;
	carrier.duty_cycle = (carrierDutyPct >= 20 && carrierDutyPct <= 50) ? carrierDutyPct / 100.0f : 0.33f;
	rmt_apply_carrier(txChannel, &carrier);

	rmt_copy_encoder_config_t copyConfig{};
	rmt_encoder_handle_t encoder = nullptr;
	if(rmt_new_copy_encoder(&copyConfig, &encoder) != ESP_OK){
		ESP_LOGE(TAG, "Failed to create RMT copy encoder");
		rmt_del_channel(txChannel);
		return;
	}

	rmt_enable(txChannel);

	uint32_t frameDurationUs = 0;
	for(uint16_t i = 0; i < frameCount; ++i){
		frameDurationUs += frame[i].duration0 + frame[i].duration1;
	}

	const uint8_t sends = std::max<uint8_t>(sendCount, 1);
	const uint32_t period = (periodUs != 0) ? periodUs : DefaultRepeatPeriodUs;

	rmt_transmit_config_t txCfg{};
	txCfg.loop_count = 0;

	for(uint8_t i = 0; i < sends; ++i){
		rmt_transmit(txChannel, encoder, frame, frameCount * sizeof(rmt_symbol_word_t), &txCfg);
		if(rmt_tx_wait_all_done(txChannel, pdMS_TO_TICKS(500)) != ESP_OK){
			ESP_LOGE(TAG, "TX did not finish in time");
			break;
		}

		if(i + 1 < sends){
			uint32_t gapMs = (period > frameDurationUs) ? (period - frameDurationUs) / 1000 : 0;
			gapMs = std::max(gapMs, MinInterFrameGapMs);
			delayMillis(gapMs);
		}
	}

	rmt_disable(txChannel);
	rmt_del_encoder(encoder);
	rmt_del_channel(txChannel);
}
