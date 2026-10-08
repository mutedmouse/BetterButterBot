#ifndef CMF_LVSELECTABLE_H
#define CMF_LVSELECTABLE_H

#include <lvgl.h>
#include "LVObject.h"

class LVSelectable : public LVObject {
public:
	LVSelectable(lv_obj_t* parent);
	virtual ~LVSelectable();

	bool isActive() const;
	void select();
	void deselect();

protected:
	lv_group_t* inputGroup;

	virtual void onSelect(){}

	virtual void onDeselect(){}

private:
	lv_group_t* parentGroup = nullptr;
	bool active = false;
};

#endif //CMF_LVSELECTABLE_H
