#include "../freertospp/Task.hpp"
#include "protocol.hpp"
#include "../network/tcp_sock.hpp"
#include "esp_sntp.h"

namespace server {

static void errno_throw(ssize_t res) {
	if (res < 0) {
		throw std::runtime_error(strerror(errno));
	}
}

class CommTask: public Task {
	static constexpr const char *TAG = "server:task";

	TCP_sock socket;
	Protocol proto;

	void task() override {
		while(true) {
			try {
				establish();
				while(true) {
					proto.begin_package(1);
					auto calc = proto.begin_packet(sizeof(proto.dummy_data));
					proto.write_dummy_data(calc);
					proto.end_packet(calc);
					proto.end_package();
					socket.write(proto.output());

					uint8_t data[32];
					auto len = socket.read_all(data, 32, pdMS_TO_TICKS(15000));
					if (len) {
						for(ssize_t i = 0; i < len; ++i) {
							printf("%02X", data[i]);
						}
						printf("\n");
					} else {
						ESP_LOGI(TAG, "no response");
					}
				}
			} catch (const std::exception &e) {
				ESP_LOGE(TAG, "exception: %s; retrying", e.what());
			}
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	};

	//	void asda() {
	////		sntp_sync_time();
	//	}

	void fill_segment(segment seg) {
		socket.read_all(seg.data, seg.size);
	}

	void establish() {
		ESP_LOGI(TAG, "connecting");
		socket = TCP_sock("185.213.0.24", 20623);
		socket.set_read_timeout(1000);
		proto.reset();
		proto.build_header2();
		socket.write(proto.output());
		fill_segment(proto.header2_response_buffer());
		auto t = proto.header2_response_parse();
		timeval tv = {
			.tv_sec = t / 1000,
			.tv_usec = (suseconds_t)(t % 1000) * 1000,
		};
		sntp_sync_time(&tv);
		ESP_LOGI(TAG, "ok");
	}
public:
	CommTask() {}
};

}
