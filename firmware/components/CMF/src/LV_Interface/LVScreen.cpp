#include "LVScreen.h"
#include "LVGL.h"
#include "InputLVGL.h"
#include "Log/Log.h"
#include <cstdio>
#include <utility>

DEFINE_LOG(LVScreen)

LVScreen::LVScreen() : LVObject(nullptr){

	lv_obj_add_event_cb(obj, [](lv_event_t* event){
		auto screen = static_cast<LVScreen*>(lv_event_get_user_data(event));
		lv_indev_set_group(InputLVGL::getInstance()->getIndev(), screen->inputGroup);
		screen->onStart();
		screen->loaded = true;
	}, LV_EVENT_SCREEN_LOADED, this);

	inputGroup = lv_group_create();
}

LVScreen::~LVScreen(){
	if(running){
		CMF_LOG(LVScreen, LogLevel::Error, "Destroying while still running! Call stop() first.");
		abort();
	}
	lv_group_delete(inputGroup);
}

void LVScreen::transition(std::function<std::unique_ptr<LVScreen>()> create, lv_screen_load_anim_t anim){
	if(lvgl == nullptr){
		CMF_LOG(LVScreen, LogLevel::Error, "Starting transition, but LVGL ptr isn't set");
		abort();
	}
	lvgl->startScreen(std::move(create), anim);
}

void LVScreen::start(LVGL* lvgl){
	this->lvgl = lvgl;
	if(running){
		CMF_LOG(LVScreen, LogLevel::Warning, "start() called while already running");
		return;
	}
	onStarting();
	running = true;
}

void LVScreen::stop(){
	if(!running) return;
	running = false;
	loaded = false;
	onStop();
}

void LVScreen::onStarting(){}

void LVScreen::onStart(){}

void LVScreen::onStop(){}

void LVScreen::loop(){}

bool LVScreen::isRunning() const{
	return running;
}

lv_group_t* LVScreen::getInputGroup() const{
	return inputGroup;
}
