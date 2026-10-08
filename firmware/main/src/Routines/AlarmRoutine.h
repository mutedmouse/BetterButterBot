#ifndef BUTTERBOT_FIRMWARE_ALARMROUTINE_H
#define BUTTERBOT_FIRMWARE_ALARMROUTINE_H

#include <string>
#include "Routine.h"

/**
 * An alarm or timer from the voice gateway's agent: sings a song (the butter song for alarms,
 * the toast song for timers, from /spiffs/alarm/) and then announces it, repeating a few rounds
 * until the power button is pressed.
 * Routines are built by factory function pointer, so the text is handed over via setText().
 */
class AlarmRoutine : public Routine {
public:
	using Routine::Routine;

	/** song: "timer" for the toast song; anything else is an alarm (butter song). */
	static void setText(const std::string& text, const std::string& song = "");

	virtual TickingState tick(float deltaTime) override;

private:
	static constexpr const char* AlarmSongPath = "/spiffs/alarm/butter_song.aac";
	static constexpr const char* TimerSongPath = "/spiffs/alarm/toast_song.aac";
	static constexpr int Rounds = 3;

	static std::string pending;
	static bool pendingTimer;

	/** Wait for the current audio to end; true if the button was pressed meanwhile (audio stopped). */
	static bool waitOrDismiss(class Audio* audio);
	static bool buttonDown();
};

#endif //BUTTERBOT_FIRMWARE_ALARMROUTINE_H
