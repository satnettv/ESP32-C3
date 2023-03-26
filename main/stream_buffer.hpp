#pragma once
#include <cstring>
#include <stdexcept>
#include <cstdarg>
#include "segment.hpp"

class stream_buffer: public segment {
protected:
	size_t end_pos = 0;
	size_t parse_pos = 0;

public:
	stream_buffer(uint8_t *data, size_t size): segment(data, size) {

	}

	void dump() {
		printf("Buf.dump %.*s \n/DUMP\n", end_pos, data);
	}

	segment rest() {
		return segment(data + end_pos, size - end_pos);
	}

	segment contents() {
		return segment(data, end_pos);
	}


	bool advance_end(size_t count) {
		end_pos += count;
		return end_pos < size;
	}

	void shift() {
		size_t i = 0;
		for (; parse_pos < end_pos; ++i, ++parse_pos) {
			data[i] = data[parse_pos];
		}
		parse_pos = 0;
		end_pos = i;
	}

	void reset() {
		end_pos = 0;
	}

	void write(char ch) {
		data[end_pos++] = ch;
	}

	void write(const char *str) {
		while(*str) {
			data[end_pos] = *str;
			++str; ++end_pos;
		}
	}

	void write(const char *str, size_t s) {
		write((const uint8_t*)str, s);
	}
	void write(const uint8_t *str, size_t s) {
		s += end_pos;
		while(end_pos < s) {
			data[end_pos] = *str;
			++str; ++end_pos;
		}
	}

	void write_f(const char* fmt, ...) {
	    va_list args;
	    va_start(args, fmt);
		end_pos += vsprintf((char*)data + end_pos, fmt, args);
		va_end(args);
	}

	void write(const segment &seg) {
	    write(seg.data, seg.size);
	}

	bool read_exact(segment seg) {
		if (end_pos - parse_pos < seg.size) {
			return false;
		}
		auto ppos = parse_pos;
		for (auto x: seg) {
			if (x != data[parse_pos++]) {
				parse_pos = ppos;
				return false;
			}
		}
		return true;
	}
};
