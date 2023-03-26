#pragma once
#include "pins.hpp"
#include "driver/uart.h"
#include "stream_buffer.hpp"
#include "Mutex.hpp"
#include <unordered_map>
#include "freertos/freeRTOS.h"
#include "freertospp/Task.hpp"
#include "fs.hpp"
#include <functional>

class GPS: public Task {
protected:
	QueueHandle_t uart_queue;
	struct Buffer {
		static constexpr size_t size = 83;
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
//			printf("%.*s", buffer.end_pos, buffer.buf);
		}
		return buffer.buf[buffer.parse_pos++];
	}

	int dig() {
		return ch() - '0';
	}
public:
	static constexpr const char *TAG = "GPS";
	static constexpr const uart_port_t uart_num = UART_NUM_0;

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

		const int uart_buffer_size = (1024 * 2);

		ESP_ERROR_CHECK(uart_driver_install(uart_num, uart_buffer_size, 0, 10, &uart_queue, 0));
		Task::start("gps", 2048);
	}

	void task() override {
		char c;
		while(true) {
			while(ch() != '$') {}
			while((c = ch()) != ',') {
				parse_buffer.write(c);
			}
			if (parse_buffer.match("GPGGA")) {
				parse_buffer.reset();
				parse_gga();
			} else {
				while((c = ch()) != '\r') {}
			}
		}
	}

	void parse_gga() {
		while(ch() != ',') {} // time; 2
		parse_lat(); // 3,4
	}

	void dump(FILE_RAII &f) {
		auto r = data.get();
		for (auto &kv: *r) {
			f.printf("%.*s,%.*s\n", kv.first.size(), kv.first.data(), kv.second.size(), kv.second.data());
		}
	}

	void dump() {
		auto r = data.get();
		for (auto &kv: *r) {
			ESP_LOGI(TAG, "%.*s,%.*s", kv.first.size(), kv.first.data(), kv.second.size(), kv.second.data());
		}
	}

	template <size_t S> void dump(char (&buf)[S], std::function<void()> f) {
		auto r = data.get();
		for (auto &kv: *r) {
			snprintf(buf, S, "%.*s,%.*s", kv.first.size(), kv.first.data(), kv.second.size(), kv.second.data());
			f();
		}
	}
};
