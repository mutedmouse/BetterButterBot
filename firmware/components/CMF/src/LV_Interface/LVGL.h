#ifndef CMF_LVGL_H
#define CMF_LVGL_H

#include <lvgl.h>
#include "LVScreen.h"
#include "Devices/Display.h"
#include "Entity/AsyncEntity.h"

class LVGL : public AsyncEntity {
	GENERATED_BODY(LVGL, AsyncEntity, CONSTRUCTOR_PACK(Display*, std::function<lv_theme_t*(lv_disp_t*)>))

public:
	LVGL(Display* display, std::function<lv_theme_t*(lv_disp_t*)> themeInit = {});

	/**
	 * Queues a screen transition. The actual destruction of the current screen
	 * and construction of the new one is deferred to the next tick(), so it is
	 * safe to call from inside an EventHandle callback (which would otherwise
	 * destroy the EventHandle's owning screen while still iterating its scan
	 * queue) or from an LVGL timer/animation callback.
	 */
	void startScreen(std::function<std::unique_ptr<LVScreen>()> create, lv_screen_load_anim_t anim = LV_SCR_LOAD_ANIM_NONE);

	/** startScreen should be called immediately after this function. */
	void stopScreen();

	void resume();

	/** Draw .bin image to display */
	static void drawImage(const char* src);

	void resetDisplayRefreshTimer();

	/** Starts the LVGL thread. Must be called after startScreen(). */
	void startThread() noexcept;

protected:
	void begin() noexcept override;

private:
	Display* display;

	lv_disp_t* lvDisplay;

	void tick(float deltaTime) noexcept override;

	static void flush(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map);

	std::unique_ptr<LVScreen> currentScreen;

	std::function<std::unique_ptr<LVScreen>()> nextScreenCreate;
	lv_screen_load_anim_t nextScreenAnim = LV_SCR_LOAD_ANIM_NONE;

	void applyPendingScreen();

	SemaphoreHandle_t initSem;
};

#endif //CMF_LVGL_H
