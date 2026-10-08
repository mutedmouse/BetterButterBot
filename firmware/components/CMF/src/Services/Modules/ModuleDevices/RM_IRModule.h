#ifndef CMF_RM_IRMODULE_H
#define CMF_RM_IRMODULE_H

#include "Object/Class.h"
#include "Services/Modules/ModuleDevice.h"
#include <driver/gpio.h>
#include <hal/rmt_types.h>
#include <cstdint>

static constexpr uint16_t IRMaxSymbols = 128;      // demodulated envelope symbols (storage/replay)
static constexpr uint16_t IRRawMaxSymbols = 1536;  // raw carrier cycles per frame (the module's receiver does not demodulate)
static constexpr uint16_t RMTMemBlockSymbols = 64; // 2 HW blocks × 48 words, fits most protocols
static constexpr uint8_t IRMaxCaptureFrames = 8;

struct IRSignal {
    rmt_symbol_word_t symbols[IRMaxSymbols];
    uint16_t count = 0;
};

// One raw frame: one symbol per carrier cycle, burst-final cycles carry the envelope space in duration1.
struct IRRawFrame {
    rmt_symbol_word_t symbols[IRRawMaxSymbols];
    uint16_t count = 0;
};

struct IRCapture {
    IRRawFrame frames[IRMaxCaptureFrames];
    int64_t doneTimestampsUs[IRMaxCaptureFrames];
    uint8_t count = 0;
};

class RM_IRModule : public ModuleDevice {
	GENERATED_BODY(RM_IRModule, ModuleDevice, CONSTRUCTOR_PACK(const Modules::BusPins&))
public:
	RM_IRModule(const Modules::BusPins& busPins = {});

	void write(bool state);
	bool read();

	gpio_num_t getInputGPIO() const;
	gpio_num_t getOutputGPIO() const;

	/**
	 * Captures a burst of raw (non-demodulated) IR frames while the user holds a remote button.
	 * Frames are separated by >=10ms of idle; done-timestamps are recorded for period measurement.
	 * Waits firstFrameTimeoutMs for the first frame, then collects for burstWindowMs.
	 * @return true if at least one frame was captured.
	 */
	bool captureBurst(IRCapture& out, uint32_t firstFrameTimeoutMs, uint32_t burstWindowMs = 2000);

	/**
	 * Sends an IR command (envelope symbols) sendCount times (min 1) at periodUs intervals;
	 * periodUs 0 falls back to 45 ms. carrierHz/carrierDutyPct outside 30-60 kHz / 20-50%
	 * fall back to 38 kHz / 33%.
	 */
	void sendIR(const rmt_symbol_word_t* frame, uint16_t frameCount, uint8_t sendCount,
				uint32_t periodUs, uint32_t carrierHz = 0, uint8_t carrierDutyPct = 0);
};


#endif //CMF_RM_IRMODULE_H
