#include "IRStorage.h"
#include <cstring>
#include <nvs_flash.h>
#include <Log/Log.h>

DEFINE_LOG(IRStorage)

IRStorage::IRStorage(){
    ESP_ERROR_CHECK(nvs_open(NVSNamespace, NVS_READWRITE, &handle));
    load();
}

IRStorage::~IRStorage(){
    nvs_close(handle);
}

uint8_t IRStorage::getCount() const{
    return data.count;
}

bool IRStorage::isFull() const{
    return data.count >= MaxIRCommands;
}

const IRCommand& IRStorage::getCommand(uint8_t index) const{
    return data.commands[index];
}

bool IRStorage::add(const char* phonemes, const ProcessedIR& ir){
    if(isFull() || !phonemes || ir.frameCount == 0 || ir.frameCount > IRMaxSymbols){
        return false;
    }

    uint8_t slot = 0;
    while(slot < MaxIRCommands && slotInUse(slot)){
        slot++;
    }

    IRCommand& cmd = data.commands[data.count];
    memset(&cmd, 0, sizeof(IRCommand));
    strncpy(cmd.phonemes, phonemes, MaxIRPhonemeLen - 1);
    cmd.phonemes[MaxIRPhonemeLen - 1] = '\0';
    memcpy(cmd.frame, ir.frame, ir.frameCount * sizeof(rmt_symbol_word_t));
    cmd.frameCount = ir.frameCount;
    cmd.sendCount = ir.sendCount;
    cmd.periodUs = ir.periodUs;
    cmd.carrierHz = ir.carrierHz;
    cmd.carrierDutyPct = ir.carrierDutyPct;

    slotOf[data.count] = slot;
    dirtySlots |= 1u << slot;
    data.count++;
    return true;
}

bool IRStorage::remove(uint8_t index){
    if(index >= data.count){
        return false;
    }
    dirtySlots |= 1u << slotOf[index];
    for(uint8_t i = index; i + 1 < data.count; ++i){
        data.commands[i] = data.commands[i + 1];
        slotOf[i] = slotOf[i + 1];
    }
    data.count--;
    return true;
}

void IRStorage::clear(){
    data = StorageData{};
    // All slots, not just mapped ones, so stray/corrupt blobs get erased too.
    dirtySlots = (1u << MaxIRCommands) - 1;
}

void IRStorage::store(){
    for(uint8_t slot = 0; slot < MaxIRCommands; ++slot){
        if(!(dirtySlots & (1u << slot))) continue;

        char key[4];
        nvsKeyForSlot(slot, key);

        uint8_t ramIndex = 0;
        esp_err_t err;
        if(slotInUse(slot, &ramIndex)){
            err = nvs_set_blob(handle, key, &data.commands[ramIndex], sizeof(IRCommand));
        } else {
            err = nvs_erase_key(handle, key);
            if(err == ESP_ERR_NVS_NOT_FOUND){
                err = ESP_OK;
            }
        }

        if(err != ESP_OK){
            // slot stays dirty and is retried on the next store
            CMF_LOG(IRStorage, LogLevel::Error, "Failed to sync IR slot %d: %s", slot, esp_err_to_name(err));
            continue;
        }
        dirtySlots &= ~(1u << slot);
    }

    nvs_commit(handle);
}

void IRStorage::setSelectedIndex(uint8_t index){
    selectedIndex = index;
}

uint8_t IRStorage::getSelectedIndex() const{
    return selectedIndex;
}

void IRStorage::nvsKeyForSlot(uint8_t slot, char* out){
    out[0] = 'c';
    out[1] = '0' + (slot % 10);
    out[2] = '\0';
}

bool IRStorage::slotInUse(uint8_t slot, uint8_t* ramIndex) const{
    for(uint8_t i = 0; i < data.count; ++i){
        if(slotOf[i] == slot){
            if(ramIndex) *ramIndex = i;
            return true;
        }
    }
    return false;
}

void IRStorage::load(){
    // Discard pre-release layouts: the monolithic "commands" blob, and any slots the legacy
    // "cnt" key hid.
    bool migrated = nvs_erase_key(handle, LegacyBlobName) == ESP_OK;
    uint8_t legacyCount = 0;
    if(nvs_get_u8(handle, LegacyCountKey, &legacyCount) == ESP_OK){
        for(uint8_t slot = legacyCount; slot < MaxIRCommands; ++slot){
            char key[4];
            nvsKeyForSlot(slot, key);
            nvs_erase_key(handle, key);
        }
        nvs_erase_key(handle, LegacyCountKey);
        migrated = true;
    }
    if(migrated){
        nvs_commit(handle);
    }

    data = StorageData{};
    for(uint8_t slot = 0; slot < MaxIRCommands; ++slot){
        char key[4];
        nvsKeyForSlot(slot, key);

        IRCommand& cmd = data.commands[data.count];
        size_t len = sizeof(IRCommand);
        const esp_err_t err = nvs_get_blob(handle, key, &cmd, &len);
        if(err == ESP_ERR_NVS_NOT_FOUND){
            continue;
        }
        if(err != ESP_OK || len != sizeof(IRCommand) || cmd.frameCount == 0 || cmd.frameCount > IRMaxSymbols){
            CMF_LOG(IRStorage, LogLevel::Warning, "Dropping invalid IR command in slot %d", slot);
            dirtySlots |= 1u << slot; // erase on the next store
            continue;
        }
        cmd.phonemes[MaxIRPhonemeLen - 1] = '\0';
        slotOf[data.count] = slot;
        data.count++;
    }
}
