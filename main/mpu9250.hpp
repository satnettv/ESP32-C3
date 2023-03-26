#pragma once
#include "i2c.hpp"
#include <cmath>
#include "fs.hpp"

class MPU9250: public I2C::dev {
protected:
public:
	static constexpr const char *TAG = "MPU9250";
	MPU9250() {
		addr = 0b1101000;
	}
	void setup() {
		write_reg<uint8_t>(107, 1 << 7); // full reset
		write_reg<uint8_t>(28, 0); // accel full scale is 2g
//		write_reg(29, 7 | 1 << 3); // accel bw 420Hz
//		write_reg<uint8_t>(106, 1 << 6 | 1 << 2); // fifo enable; fifo reset 1 << 2
//		write_reg<uint8_t>(35, 1 << 3); // accel fifo enable
	}
	uint16_t fifo_count() {
		return read_reg<uint16_t>(114) & 0xFFF;
	}
	void fifo_read(segment seg) {
		read_reg(116, seg);
	}
	struct vec3d {
		float x, y, z;
	};
	vec3d accel_read() {
		uint8_t buf[6];
		segment seg(buf, 6);
		read_reg(59, seg);
		vec3d out;
		out.x = (float)(int16_t)(buf[0] << 8 | buf[1]) / 16384.;
		out.y = (float)(int16_t)(buf[2] << 8 | buf[3]) / 16384.;
		out.z = (float)(int16_t)(buf[4] << 8 | buf[5]) / 16384.;
		return out;
	}

	static float accel_tilt(vec3d accel_g) {
		auto len_xy_z = sqrt(pow(accel_g.x, 2) + pow(accel_g.y, 2));
		return atan2(len_xy_z, accel_g.z) * 180 / M_PI;
	}

	void dump_tilt(FILE_RAII &f) {
		auto data = accel_read();
		f.printf("tilt = %.02f\n", accel_tilt(data));
	}

	template <size_t S> void dump_tilt(char (&buf)[S]) {
		auto data = accel_read();
		snprintf(buf, S, "tilt = %.02f", accel_tilt(data));
	}

	void dump_tilt() {
		auto data = accel_read();
		ESP_LOGI(TAG, "tilt = %.02f", accel_tilt(data));
	}
};
