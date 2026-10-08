#include "InputLVGL.h"
#include <utility>
#include "Services/ButtonInput.h"
#include "Core/Application.h"

InputLVGL* InputLVGL::instance = nullptr;

InputLVGL::InputLVGL(ButtonInput* bi, const std::map<Enum<int>, lv_key_t>& keyMap, const std::map<lv_key_t, lv_key_t>& vertRemap) :
		keyMap(keyMap), vertRemap(vertRemap), bi(bi){
	instance = this;

	inputDevice = lv_indev_create(); // TODO this causes a crash
	lv_indev_set_type(inputDevice, LV_INDEV_TYPE_ENCODER);
	lv_indev_set_read_cb(inputDevice, [](lv_indev_t* drv, lv_indev_data_t* data){ InputLVGL::getInstance()->read(drv, data); });
	lv_indev_set_long_press_time(inputDevice, 350);
}

void InputLVGL::postInitProperties() noexcept{
	bi->OnButtonEvent.bind(this, &InputLVGL::onButtonInput);
}

void InputLVGL::read(lv_indev_t* drv, lv_indev_data_t* data) const {
	if(!keyMap.contains(lastKey)) {
		return;
	}

	auto key = keyMap.at(lastKey);
	if(vertNav && vertRemap.contains(key)){
		key = vertRemap.at(key);
	}

	data->key = key;
	data->state = (action == ButtonInput::Action::Press) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

InputLVGL* InputLVGL::getInstance(){
	return instance;
}

void InputLVGL::onButtonInput(Enum<int> btn, ButtonInput::Action btnAction){
	lastKey = std::move(btn);
	action = btnAction;
}

lv_indev_t* InputLVGL::getIndev() const{
	return inputDevice;
}

void InputLVGL::setVertNav(bool vertNav){
	InputLVGL::vertNav = vertNav;
}