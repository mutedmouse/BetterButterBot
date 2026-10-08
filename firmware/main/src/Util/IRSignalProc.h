#ifndef BUTTERBOT_FIRMWARE_IRSIGNALPROC_H
#define BUTTERBOT_FIRMWARE_IRSIGNALPROC_H

#include <hal/rmt_types.h>
#include <Services/Modules/ModuleDevices/RM_IRModule.h>
#include <cstdint>

struct ProcessedIR {
    rmt_symbol_word_t frame[IRMaxSymbols];
    uint16_t frameCount = 0;
    uint8_t sendCount = 1;  // how many times to transmit the frame for a single press
    uint32_t periodUs = 0;  // repeat period; 0 = unknown
    uint32_t carrierHz = 0; // measured carrier; 0 = unknown, TX falls back to 38 kHz
    uint8_t carrierDutyPct = 0;
};

/**
 * Distills a raw IR burst into a replayable command: demodulates carrier cycles into
 * envelope frames (measuring carrier frequency/duty), drops noise, clusters frames and
 * averages the main cluster, detects distinct repeat frames (NEC-style) and measures
 * the repeat period.
 * @return false if the capture looks like noise or an unsupported protocol.
 */
bool processCapture(const IRCapture& cap, ProcessedIR& out);

#endif //BUTTERBOT_FIRMWARE_IRSIGNALPROC_H
