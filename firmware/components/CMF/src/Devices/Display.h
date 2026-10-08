#ifndef CMF_DISPLAY_H
#define CMF_DISPLAY_H

#include "Object/Object.h"
#include <LovyanGFX.h>
#include <Object/Class.h>

typedef lgfx::LGFX_Sprite Sprite;

/**
 * Wrapper for LovyanGFX objects.
 */
class Display : public Object {
	GENERATED_BODY(Display, Object, CONSTRUCTOR_PACK(lgfx::Bus_SPI::config_t, lgfx::Panel_Device::config_t, std::function<void(Sprite&)>))

public:
	Display(lgfx::Bus_SPI::config_t busConfig, lgfx::Panel_Device::config_t panelConfig, std::function<void(Sprite&)> canvasInit = {});
	~Display() override;

	LGFX_Device& getLGFX();

	void commit();

	Sprite& getCanvas();

	void drawTest();

private:
	lgfx::Bus_SPI bus;
	lgfx::Panel_ST7735S panel;
	LGFX_Device lgfx;

	Sprite canvas;
};

#endif //CMF_DISPLAY_H
