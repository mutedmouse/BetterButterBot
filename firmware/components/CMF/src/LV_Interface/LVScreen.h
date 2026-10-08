#ifndef CMF_LVSCREEN_H
#define CMF_LVSCREEN_H

#include "LVObject.h"
#include <unordered_set>
#include <functional>
#include <memory>

class LVGL;

class LVScreen : public LVObject {
public:
	LVScreen();
	virtual ~LVScreen();

	lv_group_t* getInputGroup() const;

	bool isRunning() const;

protected:
	lv_group_t* inputGroup;

	void transition(std::function<std::unique_ptr<LVScreen>()> create, lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE);

private:
	LVGL* lvgl = nullptr;

	void start(LVGL* lvgl);
	void stop();

	virtual void onStarting();
	virtual void onStart();
	virtual void onStop();

	bool running = false;
	bool loaded = false;

	friend LVGL;
	virtual void loop();
};

#endif //CMF_LVSCREEN_H
