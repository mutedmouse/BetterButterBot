#ifndef BUTTERBOT_FIRMWARE_IRSTORAGE_H
#define BUTTERBOT_FIRMWARE_IRSTORAGE_H

#include <nvs.h>
#include <hal/rmt_types.h>
#include <cstdint>
#include "Util/IRSignalProc.h"

static constexpr uint8_t MaxIRCommands = 10;
static constexpr uint8_t MaxIRPhonemeLen = 64;

struct IRCommand {
    char phonemes[MaxIRPhonemeLen];
    rmt_symbol_word_t frame[IRMaxSymbols];
    uint16_t frameCount;
    uint8_t sendCount;      // times to transmit the frame for a single press
    uint32_t periodUs;      // repeat period; 0 = unknown
    uint32_t carrierHz;     // 0 = unknown, TX falls back to 38 kHz
    uint8_t carrierDutyPct; // 0 = unknown, TX falls back to 33%
};

/**
 * Persists IR commands as one NVS blob per stable slot ("c0".."c9"). Slots are never shifted,
 * so an NVS error can at worst lose an add or resurrect a remove — never mix two commands.
 * RAM order may change across reboots (load compacts in slot order); nothing depends on it.
 */
class IRStorage {
public:
    IRStorage();
    virtual ~IRStorage();

    uint8_t getCount() const;
    bool isFull() const;
    const IRCommand& getCommand(uint8_t index) const;
    bool add(const char* phonemes, const ProcessedIR& ir);
    bool remove(uint8_t index);
    void clear();
    void store();

    void setSelectedIndex(uint8_t index);
    uint8_t getSelectedIndex() const;

private:
    struct StorageData {
        uint8_t count = 0;
        IRCommand commands[MaxIRCommands];
    };

    StorageData data;
    uint8_t slotOf[MaxIRCommands]{}; // RAM index -> NVS slot
    uint8_t selectedIndex = 0;

    uint16_t dirtySlots = 0; // NVS slots needing sync (write if mapped, erase otherwise)

    static constexpr const char* NVSNamespace = "BB_IR";
    static constexpr const char* LegacyCountKey = "cnt";
    static constexpr const char* LegacyBlobName = "commands";
    nvs_handle_t handle{};

    static void nvsKeyForSlot(uint8_t slot, char* out);
    bool slotInUse(uint8_t slot, uint8_t* ramIndex = nullptr) const;
    void load();
};

#endif //BUTTERBOT_FIRMWARE_IRSTORAGE_H
