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

//	segment header2_response_buffer() {
//		assert(pos == 0);
//		return segment(buf, 9);
//	}
//	uint32_t header2_response_parse(std::function<void(segment)> reader) {
//		auto res = server_com_parse(reader);
//		if (res.parcel_number != 0 || res.data.size != 4) {
//			throw std::runtime_error("unexpected response to header");
//		}
//		auto t = read<uint32_t>(res.data);
//		reset();
//		return t;
////		assert(pos == 0);
////		if (buf[pos++] != 0x7B) {
////			goto error;
////		}
////		if (buf[pos++] != 0x04) {
////			goto error;
////		}
////		++pos; // parcel number ??
////		++pos; // checksum
////		{
////			auto timestamp = read<uint32_t>(4);
////			printf("%lu\n", timestamp);
////			if (buf[pos++] != 0x7D) {
////				goto error;
////			}
////			output().dump_hex();
////			pos = 0;
////			return timestamp;
////		}
////		error:
////		throw std::runtime_error(std::string("unexpected response; pos = ") +
////				std::to_string(pos - 1) + "; value = " + std::to_string(buf[pos-1]));
//	}


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
				throw std::runtime_error("unexpected response to header");
			}
			return read<uint32_t>(data);
		}
	};
	friend Server_com;

	Server_com server_com() {
		return Server_com(*this);
	}

	void begin_package(uint8_t parcel_number) {
		buf[pos++] = 0x5B;
		buf[pos++] = parcel_number;
	}

	void end_package() {
		buf[pos++] = 0x5D;
	}

	chkcalc begin_packet(uint16_t length) {
		chkcalc chk;
		buf[pos++] = 0x01;
		write<uint16_t>(length, 2);
		write<uint32_t>(time(0) * 1000, 4, chk);
		return chk;
	}

	void end_packet(chkcalc &chk) {
		buf[pos++] = chk.get();
	}


	void write_dummy_data(chkcalc &chk) {
		write_arr(dummy_data, sizeof(dummy_data), chk);
	}

	static constexpr const uint8_t dummy_data[] = {
			0xFC, 0xFF, 0x0F, 0x07, 0x00,
			0xFD, 0x67, 0xD2, 0xC8, 0x29,
			0xFE, 0x18, 0xA7, 0x95, 0x0A,
			0xFF, 0xA2, 0x68, 0x02, 0x00
	};
};

}
