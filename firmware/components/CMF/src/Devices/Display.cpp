#include "Display.h"

Display::Display(lgfx::Bus_SPI::config_t busConfig, lgfx::Panel_Device::config_t panelConfig, std::function<void(Sprite&)> canvasInit) : canvas(&lgfx){
	bus.config(busConfig);
	panel.config(panelConfig);
	panel.setBus(&bus);

	// LGFX init -> panel init -> bus init
	panel.setBus(&bus);
	lgfx.setPanel(&panel);
	lgfx.init();

	if(canvasInit){
		canvasInit(canvas);
	}
}

Display::~Display(){
	bus.release();
}

Sprite& Display::getCanvas(){
	return canvas;
}

void Display::commit(){
	canvas.pushSprite(0, 0);
}

LGFX_Device& Display::getLGFX(){
	return lgfx;
}

void Display::drawTest(){
	printf("Draw test...\n");
	lgfx.clear(TFT_RED);
	lgfx.setCursor(10, 10);
	lgfx.printf("Hellooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo\n");
	lgfx.drawLine(0, 0, 127, 0, TFT_GREEN);
	lgfx.drawLine(0, 0, 0, 127, TFT_GREEN);
	lgfx.drawLine(0, 127, 127, 127, TFT_GREEN);
	lgfx.drawLine(127, 0, 127, 127, TFT_GREEN);
	printf("Done.\n");
}
