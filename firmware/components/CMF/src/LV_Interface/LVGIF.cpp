#include <string>
#include <cstdio>
#include "Log/Log.h"
#include "LVGIF.h"

DEFINE_LOG(LVGIF)

LVGIF::LVGIF(lv_obj_t* parent, const char* path) : LVObject(parent), path(path){
	const std::string descPath = std::string(path) + "/desc.bin";

	pathLen = descPath.length() + 12;
	imgPath = new char[pathLen];

	lv_fs_file_t f;
	if(lv_fs_open(&f, descPath.c_str(), LV_FS_MODE_RD) != LV_FS_RES_OK){
		CMF_LOG(LVGIF, LogLevel::Error, "Couldn't open GIF descriptor file at %s", descPath.c_str());
		return;
	}

	// Header: width(2) + height(2) + frame count(4), all big-endian (most significant byte first)
	uint8_t header[8];
	uint32_t br = 0;
	lv_fs_read(&f, header, sizeof(header), &br);
	if(br < sizeof(header)){
		CMF_LOG(LVGIF, LogLevel::Error, "GIF descriptor too short at %s", descPath.c_str());
		lv_fs_close(&f);
		return;
	}

	w = (header[0] << 8) | header[1];
	h = (header[2] << 8) | header[3];
	const uint32_t length = ((uint32_t) header[4] << 24) | ((uint32_t) header[5] << 16) | ((uint32_t) header[6] << 8) | header[7];

	durations.reserve(length);
	for(uint32_t i = 0; i < length; i++){
		uint8_t duration[2]; // big-endian, in 10ms units
		lv_fs_read(&f, duration, sizeof(duration), &br);
		durations.push_back(((duration[0] << 8) | duration[1]) * 10);
	}

	lv_fs_close(&f);

	lv_obj_set_size(obj, w, h);
	lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
	img = lv_img_create(obj);

	sprintf(imgPath, "%s/%lu.bin", path, index);
	lv_img_set_src(img, imgPath);
	timer = lv_timer_create([](lv_timer_t* t){
		auto gif = (LVGIF*) lv_timer_get_user_data(t);
		bool done = false;

		gif->index++;
		if(gif->index >= gif->durations.size()){
			done = true;

			if(gif->loopType == LoopType::On){
				gif->index = 0;
			}else{
				lv_timer_pause(gif->timer);
			}
		}

		if(gif->loopType != LoopType::Single || !done){
			lv_timer_set_period(gif->timer, gif->durations[gif->index]);
			lv_image_cache_drop(gif->imgPath);
			sprintf(gif->imgPath, "%s/%lu.bin", gif->path, gif->index);
			lv_img_set_src(gif->img, gif->imgPath);
		}

		if(done && gif->cb){
			gif->cb();
		}
	}, durations[index], this);
	lv_timer_pause(timer);
}

LVGIF::~LVGIF(){
	if(timer != nullptr){
		lv_timer_del(timer);
	}
	delete[] imgPath;
}

void LVGIF::start(){
	lv_timer_resume(timer);
}

void LVGIF::stop(){
	lv_timer_pause(timer);
}

void LVGIF::reset(){
	index = 0;
	sprintf(imgPath, "%s/%lu.bin", path, index);
	lv_img_set_src(img, imgPath);
	lv_timer_set_period(timer, durations[index]);
	lv_timer_reset(timer);
	lv_timer_resume(timer);
}

void LVGIF::setLooping(LVGIF::LoopType loop){
	loopType = loop;
}

void LVGIF::setLoopCallback(std::function<void()> cb){
	this->cb = cb;
}

void LVGIF::setImage(size_t index){
	this->index = index;
	sprintf(imgPath, "%s/%d.bin", path, index);
	lv_img_set_src(img, imgPath);
	lv_timer_reset(timer);
	lv_timer_resume(timer);
}

size_t LVGIF::getNumFrames() const{
	return durations.size();
}
