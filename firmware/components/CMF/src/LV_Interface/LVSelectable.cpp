#include "LVSelectable.h"
#include "InputLVGL.h"

LVSelectable::LVSelectable(lv_obj_t* parent) : LVObject(parent){
	inputGroup = lv_group_create();

	lv_obj_add_event_cb(obj, [](lv_event_t* e){
		LVSelectable* sel = static_cast<LVSelectable*>(lv_event_get_user_data(e));
		sel->select();
	}, LV_EVENT_CLICKED, this);
}

LVSelectable::~LVSelectable(){
	lv_group_delete(inputGroup);
}

void LVSelectable::select(){
	active = true;

	auto curr = lv_indev_get_group(InputLVGL::getInstance()->getIndev());
	if(curr != inputGroup){
		parentGroup = curr;
		lv_indev_set_group(InputLVGL::getInstance()->getIndev(), inputGroup);
	}

	onSelect();
}

void LVSelectable::deselect(){
	if(parentGroup != nullptr){
		lv_indev_set_group(InputLVGL::getInstance()->getIndev(), parentGroup);
	}

	parentGroup = nullptr;
	active = false;

	lv_obj_send_event(obj, LV_EVENT_READY, nullptr);

	onDeselect();
}

bool LVSelectable::isActive() const{
	return active;
}
