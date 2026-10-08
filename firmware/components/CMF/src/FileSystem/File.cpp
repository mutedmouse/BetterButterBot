#include "File.h"

File::File(FileImplPtr p) : impl(std::move(p)){

}

bool File::open() const{
	return impl != nullptr && impl->operator bool();
}

File::operator bool() const{
	return open();
}

void File::close(){
	if(!open()) return;

	impl->close();
	impl.reset();
}

size_t File::size() const{
	if(!open()) return 0;

	return impl->size();
}

bool File::operator==(const File& other) const{
	return std::string(name()) == other.name();
}

const char* File::name() const{
	if(!open()) return nullptr;

	return impl->name();
}


size_t File::read(uint8_t* buf, size_t size){
	if(!open()) return -1;

	return impl->read(buf, size);
}

int File::read(){
	if(!open()) return -1;

	uint8_t result;
	if(impl->read(&result, 1) != 1){
		return -1;
	}

	return result;
}

size_t File::write(const uint8_t* buf, size_t size){
	if(!open()) return 0;

	return impl->write(buf, size);
}

size_t File::write(uint8_t c){
	if(!open()) return 0;

	return impl->write(&c, 1);
}

void File::flush(){
	if(!open()) return;

	impl->flush();
}

bool File::seek(int pos, SeekMode mode){
	if(!open()) return false;

	return impl->seek(pos, (int) mode);
}

size_t File::position() const{
	if(!open()) return 0;

	return impl->pos();
}

int File::available(){
	if(!open()) return 0;

	return impl->size() - impl->pos();
}

int File::peek(){
	if(!open()) return -1;

	int result = read();
	if(result == -1) return -1;

	seek(-1, SeekMode::SeekCur);
	return result;
}
