#pragma once
#include "../segment.hpp"
#include <coroutine>

namespace server {

class Protocol {
protected:
	uint8_t buf[255 + 6]; //max server response
	size_t pos = 0;

	class chkcalc {
		uint8_t sum = 0;
	public:
		void feed(uint8_t val) {
			sum += val;
		}
		uint8_t get() {
			return sum;
		}
	};

public:
	enum Packet_type: uint8_t {
		DATA = 1,
		TEXT = 3,
		FILE = 4,
		DATA_BINARY = 6,
		CONFIRM_THE_COMMAND = 8,
		CONFIRM_THE_COMMAND_BY_TOKEN = 9,
		SERVICE_TEXT = 13
	};

	template <class T> void write(T num, size_t size) {
		while(size--) {
			buf[pos++] = num & 0xFF;
			num >>= 8;
		}
	}
	template <class T> void write(T num, size_t size, chkcalc &chk) {
		while(size--) {
			chk.feed(buf[pos++] = num & 0xFF);
			num >>= 8;
		}
	}
	void write_arr(const uint8_t *data, size_t size, chkcalc &chk) {
		for (size_t i = 0; i < size; ++i) {
			chk.feed(buf[pos++] = data[i]);
		}
	}

	void write_arr(const uint8_t *data, size_t size) {
		for (size_t i = 0; i < size; ++i) {
			buf[pos++] = data[i];
		}
	}

	template <class T> T read(size_t size) {
		T out = 0;
		for (size_t i = 0; i < size; ++i) {
			out |= (T)buf[pos++] << (i * 8);
		}
		return out;
	}

	template <class T> static T read(segment seg) {
		T out = 0;
		for (size_t i = 0; i < seg.size; ++i) {
			out |= (T)seg.data[i] << (i * 8);
		}
		return out;
	}

	void reset() {
		pos = 0;
	}

	segment output() {
		segment out(buf, pos);
		pos = 0;
		return out;
	}

	void build_header2() {
		uint64_t imei = 865209039777769;
		//segment uid = "123456789";
		buf[pos++] = 0xFF;
		buf[pos++] = 0x23;
		write(imei, 8);
	}


	void begin_package(uint8_t parcel_number) {
		buf[pos++] = 0x5B;
		buf[pos++] = parcel_number;
	}

	void end_package() {
		buf[pos++] = 0x5D;
	}

	chkcalc begin_packet(uint16_t length, Packet_type type) {
		chkcalc chk;
		buf[pos++] = type;
		write<uint16_t>(length, 2);
		write<uint32_t>(time(0) * 1000, 4, chk);
		return chk;
	}

	void end_packet(chkcalc &chk) {
		buf[pos++] = chk.get();
	}

	void text_packet(const char *text) {
		auto len = strlen(text);
		auto chk = begin_packet(len, TEXT);
		write_arr((const uint8_t*)text, len, chk);
		end_packet(chk);
	}

	void write_tag(uint8_t num, float data, chkcalc &chk) {
		write(num, 1, chk);
		write(*((uint32_t*)&data), 4, chk);
	}


	class Server_com {
		friend Protocol;
	protected:
		Server_com(Protocol &p) {
			assert(p.pos == 0);
			data.data = p.buf;
		}
	public:
		uint8_t parcel_number = 0;
		segment data;
		segment begin() {
			return segment(data.data, 3);
		}
		segment rest() {
			if (data[0] != 0x7B) {
				throw std::runtime_error(std::string("expected 0x7B, got ") + std::to_string(data[0]));
			}
			data.size = data[1];
			parcel_number = data[2];
			segment out(data.data + 3, 1);
			data.data += 3;
			if (data.size > 0) {
				data.data += 1;
				out.size = data.size + 2;
			}
			return out;
		}
		void finish() {
			if (data[data.size] != 0x7D) {
				throw std::runtime_error(std::string("expected 0x7D, got ") + std::to_string(data[data.size]));
			}
		}
		uint32_t com_header2() {
			if (parcel_number != 0 || data.size != 4) {
				throw std::runtime_error("unexpected response to header2");
			}
			return read<uint32_t>(data);
		}
	};
	friend Server_com;

	Server_com server_com() {
		return Server_com(*this);
	}

};

}
