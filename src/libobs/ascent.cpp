#include <ascent.hpp>

using std::string;

void InputASCENTStream::close() {
}

int InputASCENTStream::ignore() {
return -1;
}

bool InputASCENTStream::open(string filename) {
return false;
}

int InputASCENTStream::peek() {
return -1;
}

int InputASCENTStream::read(unsigned char *buffer, size_t buffer_length) {
return -1;
}

void InputASCENTStream::rewind() {
}

void ASCENTData::fill(const unsigned char *stream_buffer, bool
    fill_header_only) {
}
