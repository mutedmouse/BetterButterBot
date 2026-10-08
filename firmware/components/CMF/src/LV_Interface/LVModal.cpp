#include "LVModal.h"
#include "LVScreen.h"
#include "InputLVGL.h"

LVModal* LVModal::current = nullptr;

LVModal::LVModal(LVScreen* parent) : LVObject((lv_obj_t*) *parent), parentScreen(parent){
	delete current;
	current = this;

	inputGroup = lv_group_create();

	container = lv_obj_create(*parent);
	lv_obj_set_parent(*this, container);

	lv_obj_add_event_cb(obj, [](lv_event_t* event){
		auto container = (lv_obj_t*) lv_event_get_user_data(event);
		lv_obj_delete_async(container);
	}, LV_EVENT_DELETE, container);

	lv_obj_set_size(container, 115, 82);
	lv_obj_set_style_pad_all(container, 8, 0);
	lv_obj_set_style_pad_left(container, 5, 0);
	lv_obj_set_style_pad_bottom(container, 5, 0);
	lv_obj_set_style_bg_image_src(container, "S:/Modal.bin", 0);
	lv_obj_set_style_bg_image_opa(container, LV_OPA_COVER, 0);

	lv_obj_set_size(*this, 86, 76);
	lv_obj_set_align(*this, LV_ALIGN_CENTER);

	lv_obj_set_align(container, LV_ALIGN_CENTER);
	lv_obj_add_flag(container, LV_OBJ_FLAG_FLOATING);

	oldGroup = lv_indev_get_group(InputLVGL::getInstance()->getIndev());
	lv_indev_set_group(InputLVGL::getInstance()->getIndev(), inputGroup);
}

LVModal::~LVModal(){
	lv_indev_set_group(InputLVGL::getInstance()->getIndev(), oldGroup);
	lv_group_delete(inputGroup);
	current = nullptr;
}

void LVModal::setBg(const char* src){
	lv_obj_set_style_bg_image_src(container, src, 0);
}
