#include "../freertospp/Task.hpp"
#include "protocol.hpp"
#include "../network/tcp_sock.hpp"
#include "esp_sntp.h"
#include <optional>
#include "../GPS.hpp"

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
				uint8_t i = 0;
				while(true) {
					if (++i == 0xFC) {
						i = 1;
					}
					Timeout t(pdMS_TO_TICKS(15000));

					proto.begin_package(i);
					write_gps_packet();
					proto.end_package();
					auto sz = socket.write(proto.output());
					ESP_LOGI(TAG, "sent parcel %d; size = %d", i, sz);

					while (!t.done()) {
						auto rp = read_response(t);
						if (rp) {
							if (rp->parcel_number > 0 && rp->parcel_number <= 0xFB) {
								ESP_LOGI(TAG, "parcel %d confirmed", rp->parcel_number);
							} else {
								ESP_LOGI(TAG, "command from server; parcel number = 0x%02X, size = %d", rp->parcel_number, rp->data.size);
							}
						}
					}
				}
			} catch (const std::exception &e) {
				ESP_LOGE(TAG, "exception: %s; retrying", e.what());
			}
			vTaskDelay(pdMS_TO_TICKS(10000));
		}
	};

	void fill_segment(segment seg, TickType_t timeout = portMAX_DELAY) {
		if (socket.read_all(seg.data, seg.size) != seg.size) {
			throw std::runtime_error("server response timeout");
		}
	}

	std::optional<Protocol::Server_com> read_response(TickType_t start_timeout) {
		std::optional<Protocol::Server_com> out;
		if (socket.wait_readable(start_timeout)) {
			out = proto.server_com();
			fill_segment(out->begin(), pdMS_TO_TICKS(1000));
			fill_segment(out->rest(), pdMS_TO_TICKS(1000));
			out->finish();
		}
		return out;
	}

	void establish() {
		ESP_LOGI(TAG, "connecting");
		socket = TCP_sock("185.213.0.24", 20623);
		socket.set_read_timeout(1000);
		proto.reset();
		proto.build_header2();
		socket.write(proto.output());

		auto rp = read_response(pdMS_TO_TICKS(15000));
		if (!rp.has_value()) {
			throw std::runtime_error("server response timeout");
		}
		auto t = rp->com_header2();
		timeval tv = {
			.tv_sec = t / 1000,
			.tv_usec = (suseconds_t)(t % 1000) * 1000,
		};
		sntp_sync_time(&tv);
		ESP_LOGI(TAG, "ok; server time is %lu", t);
	}

	void write_gps_packet() {
		auto d = gps.data.get();
//		d->gga.dump();
		if (d->gga.valid()) {
			auto calc = proto.begin_packet(5 * 2, proto.DATA);
			proto.write_tag(3, *d->gga.lat, calc);
			proto.write_tag(4, *d->gga.lon, calc);
			proto.end_packet(calc);

			if (d->vtg.valid()) {
				uint8_t data[4];
				data[3] = *d->vtg.speed_km / 1.852;
				data[2] = *d->gga.num_sats << 4;
				data[1] = *d->gga.alt / 10;
				data[0] = *d->vtg.course_measured / 2;
				auto calc = proto.begin_packet(5, proto.DATA);
				proto.write_tag(5, data, calc);
				proto.end_packet(calc);
			}
		}
	}
public:
	CommTask() {}
};

}
