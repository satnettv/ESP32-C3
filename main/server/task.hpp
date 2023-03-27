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
					Timeout t(15000);

					proto.begin_package(1);
					auto calc = proto.begin_packet(sizeof(proto.dummy_data));
					proto.write_dummy_data(calc);
					proto.end_packet(calc);
					proto.end_package();
					socket.write(proto.output());
					ESP_LOGI(TAG, "sent parcel %d", 1);

					auto rp = proto.server_com();
					fill_segment(rp.begin());
					fill_segment(rp.rest());
					rp.finish();

					if (rp.parcel_number) {
						ESP_LOGI(TAG, "parcel %d confirmed", rp.parcel_number);
					} else {
						ESP_LOGI(TAG, "no response");
					}
					if (!t.done()) {
						vTaskDelay(t);
					}
				}
			} catch (const std::exception &e) {
				ESP_LOGE(TAG, "exception: %s; retrying", e.what());
			}
			vTaskDelay(pdMS_TO_TICKS(10000));
		}
	};

	//	void asda() {
	////		sntp_sync_time();
	//	}

	void fill_segment(segment seg, TickType_t timeout = portMAX_DELAY) {
		socket.read_all(seg.data, seg.size);
	}

	std::optional<Protocol::Server_com> read_response(Timeout &start_timeout) {
		auto rp = proto.server_com();
		fill_segment(rp.begin(), start_timeout);
		fill_segment(rp.rest());
		rp.finish();
	}

	void establish() {
		ESP_LOGI(TAG, "connecting");
		socket = TCP_sock("185.213.0.24", 20623);
		socket.set_read_timeout(1000);
		proto.reset();
		proto.build_header2();
		socket.write(proto.output());


		auto rp = proto.server_com();
		fill_segment(rp.begin());
		fill_segment(rp.rest());
		rp.finish();
		auto t = rp.com_header2();
		timeval tv = {
			.tv_sec = t / 1000,
			.tv_usec = (suseconds_t)(t % 1000) * 1000,
		};
		sntp_sync_time(&tv);
		ESP_LOGI(TAG, "ok; server time is %lu", t);
	}
public:
	CommTask() {}
};

}
