#ifndef BUTTTERBOT_COMMON_FEEDFRAME_H
#define BUTTTERBOT_COMMON_FEEDFRAME_H


//Encoded in JPEG
struct FeedFrame {
	size_t size = 0;
	void* data = nullptr;

	virtual ~FeedFrame(){
		if(data){
			free(data);
			data = nullptr;
		}
	}


	static constexpr size_t HeaderTrailerLength = 8;
	static constexpr uint8_t Header[HeaderTrailerLength] = { 0x18, 0x20, 0x55, 0xf2, 0x5a, 0xc0, 0x4d, 0xaa };
	static constexpr uint8_t Trailer[HeaderTrailerLength] = { 0x42, 0x2c, 0xd9, 0xe3, 0xff, 0xa0, 0x11, 0x01 };

	/**
	 * Frame size is sent twice, once normally and then byte-shifted according to this scheme.
	 * Used as a simple integrity check.
	 */
	static constexpr uint8_t SizeShift[4] = { 2, 3, 1, 0 };
};

#endif //BUTTTERBOT_COMMON_FEEDFRAME_H