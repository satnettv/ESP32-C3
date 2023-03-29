#pragma once
#include "pins.hpp"
#include "driver/uart.h"
#include "stream_buffer.hpp"
#include "Mutex.hpp"
#include <unordered_map>
#include "freertos/freeRTOS.h"
#include "freertospp/Task.hpp"
#include "fs.hpp"
#include <optional>

class GPS: public Task {
protected:
	QueueHandle_t uart_queue;
	struct Buffer {
		static constexpr size_t size = 83*4;
		uint8_t buf[size];
		size_t parse_pos = 0;
		size_t end_pos = 0;
	} buffer;

	struct Parse_buffer {
		static constexpr size_t size = 83;
		uint8_t buf[size];
		size_t end_pos = 0;
		void write(char ch) {
			buf[end_pos++] = ch;
		}
		bool match(const char *text) {
			for (int i = 0; i < end_pos; ++i) {
				if (buf[i] != *text++) {
					return false;
				}
			}
			return *text == 0;
		}
		void reset() {
			end_pos = 0;
		}
	} parse_buffer;

	char ch() {
		while (buffer.parse_pos == buffer.end_pos) {
			buffer.parse_pos = 0;
			auto res = uart_read_bytes(uart_num, buffer.buf, buffer.size, portMAX_DELAY);
			if (res > 0) {
				buffer.end_pos = res;
			} else {
				buffer.end_pos = 0;
			}
		}
//		if (buffer.buf[buffer.parse_pos] != '\r') {
//			printf("%c", buffer.buf[buffer.parse_pos]);
//		}
		return buffer.buf[buffer.parse_pos++];
	}

public:
	static constexpr const char *TAG = "GPS";
	static constexpr const uart_port_t uart_num = UART_NUM_0;

	struct Data {
		struct Gga {
			std::optional<uint32_t> time;
			std::optional<float> lat;
			std::optional<float> lon;
			std::optional<uint8_t> quality;
			std::optional<uint8_t> num_sats;
			std::optional<float> hdop;
			std::optional<float> alt; // meters
			std::optional<float> undulation; // meters
			std::optional<uint8_t> age;
			std::optional<uint16_t> stn_ID;
			void dump() {
				if (time) {
					printf("time = %lu\n", *time);
				} else {
					printf("no time\n");
				}
				if (lat) {
					printf("lat = %f\n", *lat);
				} else {
					printf("no lat\n");
				}
				if (lon) {
					printf("lon = %f\n", *lon);
				} else {
					printf("no lon\n");
				}
			}
			void clear() {
				*this = Gga();
			}
			bool valid() {
				return quality && *quality != 0;
			}
		} gga;
		struct Vtg {
			std::optional<float> course_measured;
			std::optional<float> course_magnetic;
			std::optional<float> speed_km;
			std::optional<char> mode;
			void dump() {
				if (speed_km) {
					printf("speed_km = %f\n", *speed_km);
				} else {
					printf("no speed_km\n");
				}
			}
			void clear() {
				*this = Vtg();
			}
			bool valid() {
				return (bool)speed_km;
			}
		} vtg;
	};

	Mutex_val<Data> data;

	void setup() {
		uart_config_t uart_config = {
		    .baud_rate = 9600,
		    .data_bits = UART_DATA_8_BITS,
		    .parity = UART_PARITY_DISABLE,
		    .stop_bits = UART_STOP_BITS_1,
		    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
		    .rx_flow_ctrl_thresh = 0,
			.source_clk = UART_SCLK_DEFAULT
		};
		ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));
		ESP_ERROR_CHECK(uart_set_pin(uart_num, -1, pins::GPS_TX, -1, -1));

		ESP_ERROR_CHECK(uart_driver_install(uart_num, 1024, 0, 10, &uart_queue, 0));
		Task::start("gps", 1024*4);
	}

	void task() override {
		char c;
		while(true) {
			while(ch() != '$') {}
			try {
				parse_buffer.reset();
				while((c = ch()) != ',') {
					parse_buffer.write(c);
				}
				if (parse_buffer.match("GPGGA")) {
					parse_buffer.reset();
					parse_gga();
				} else if (parse_buffer.match("GPVTG")) {
					parse_buffer.reset();
					parse_vtg();
				} else {
//					parse_buffer.write(0);
//					printf("got %s\n", parse_buffer.buf);
				}
			} catch (const std::exception &e) {
				ESP_LOGE(TAG, "exception %s", e.what());
			}
		}
	}

	void parse_gga() {
		auto _data = data.get();
		auto &out = _data->gga;
		try {
			out.time = parse_time();
			out.lat = parse_lat();
			out.lon = parse_lon();
			out.quality = parse_dec<uint8_t>();
			out.num_sats = parse_dec<uint8_t>();
			out.hdop = parse_float();
			out.alt = parse_float_units('M');
			out.undulation = parse_float_units('M');
			out.age = parse_dec<uint8_t>();
			out.stn_ID = parse_dec<uint16_t>();
			parse_csum();
		} catch(...) {
			out.clear();
			throw;
		}
	}

	void parse_vtg() {
		auto _data = data.get();
		auto &out = _data->vtg;
		try {
			out.course_measured = parse_float_units('T');
			out.course_magnetic = parse_float_units('M');
			parse_float_units('N'); // speed_knots
			out.speed_km = parse_float_units('K');
			out.mode = parse_char();
			parse_csum();
		} catch(...) {
			out.clear();
			throw;
		}
//		out.dump();
	}

	uint32_t digit(uint8_t ch) {
		if (ch < '0' || ch > '9') {
			throw std::runtime_error("expected digit");
		}
		return ch - '0';
	}

	std::optional<uint32_t> parse_time() {
		std::optional<uint32_t> out;
		auto c = ch();
		if (!sep(c)) {
			uint8_t h = digit(c) * 10 + digit(ch());
			uint8_t m = digit(ch()) * 10 + digit(ch());
			uint8_t s = digit(ch()) * 10 + digit(ch());
			if (ch() != '.') {
				throw std::runtime_error("expected .");
			}
			uint8_t s_dec = digit(ch()) * 10 + digit(ch());
			if (!sep(ch())) {
				throw std::runtime_error("expected separator");
			}
			out = s + (m + h * 60) * 60;
		}
		return out;
	}

	std::optional<float> parse_lat() {
		auto c = ch();
		std::optional<float> f;
		if (!sep(c)) {
			uint8_t degs = digit(c) * 10 + digit(ch());
			f = parse_float();
			read_segment_into_parse_buffer();
			if (parse_buffer.end_pos > 0) {
				*f /= 60;
				*f += degs;
				if (parse_buffer.buf[0] == 'S') {
					*f = -*f;
				}
			}
			parse_buffer.reset();
		} else {
			while (!sep(ch())) {}
		}
		return f;
	}

	std::optional<float> parse_lon() {
		auto c = ch();
		std::optional<float> f;
		if (!sep(c)) {
			uint8_t degs = digit(c) * 100 + digit(ch()) * 10 + digit(ch());
			f = parse_float();
			read_segment_into_parse_buffer();
			if (parse_buffer.end_pos > 0) {
				*f /= 60;
				*f += degs;
				if (parse_buffer.buf[0] == 'W') {
					*f = -*f;
				}
			}
			parse_buffer.reset();
		} else {
			while (!sep(ch())) {}
		}
		return f;
	}

	bool sep(char c) {
		return c == '\r' || c == ',' || c == '*';
	}

	template <class T> std::optional<T>parse_dec() {
		std::optional<T> out;
		char c = ch();
		if (!sep(c)) {
			out = (T)(c - '0');
			while(!sep(c = ch())) {
				*out *= 10;
				*out += c - '0';
			}
		}
		return out;
	}

	std::optional<float> parse_float() {
		read_segment_into_parse_buffer();
		std::optional<float> out;

		if (parse_buffer.end_pos > 0) {
			parse_buffer.write(0);
			out = atof((char*)parse_buffer.buf);
		}
		parse_buffer.reset();
		return out;
	}

	std::optional<std::pair<float, char>> parse_float_units() {
		std::optional<std::pair<float, char>> out;
		auto f = parse_float();
		read_segment_into_parse_buffer();
		if (f && parse_buffer.end_pos > 0) {
			out = {*f, parse_buffer.buf[0]};
		}
		parse_buffer.reset();
		return out;
	}

	std::optional<float> parse_float_units(char expect) {
		std::optional<float> out;
		auto fu = parse_float_units();
		if (fu) {
			if (fu->second == expect) {
				out = fu->first;
			} else {
				ESP_LOGW(TAG, "unexpected unit: %c; expected %c", fu->second, expect);
			}
		}
		return out;
	}

	std::optional<char> parse_char() {
		std::optional<char> out;
		auto c = ch();
		if (!sep(c)) {
			out = c;
			while(!sep(ch())) {}
		}
		return out;
	}

	void read_segment_into_parse_buffer() {
		char c;
		while(!sep(c = ch())) {
			parse_buffer.write(c);
		}
	}

	uint8_t parse_csum() {
		while(!sep(ch())) {

		}
		return 0;
	}
};

extern GPS gps;



/*
$GPGLL,,,,,235448.00,V,N*46
$GPRMC,235449.00,V,,,,,,,280323,,,N*78
$GPVTG,,,,,,,,,N*30
$GPGGA,235449.00,,,,,0,00,99.99,,,,,,*6B
$GPGSA,A,1,,,,,,,,,,,,,99.99,99.99,99.99*30
$GPGSV,3,1,11,03,37,146,20,04,81,091,,06,39,267,,07,24,206,*72
$GPGSV,3,2,11,09,61,264,,11,25,315,21,16,24,107,,20,02,315,*75
$GPGSV,3,3,11,26,29,066,,29,06,011,,31,11,046,*4B
*/
