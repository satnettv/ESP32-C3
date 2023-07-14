#pragma once
#include "driver/i2c.h"
#include "segment.hpp"
#include "esp_log.h"
#include "esp-exception.hpp"

class I2C {
public:
	class cmd {
	protected:
		i2c_cmd_handle_t h;
	public:
		cmd() {
			h = i2c_cmd_link_create();
			assert(h);
		}
		~cmd() {
			i2c_cmd_link_delete(h);
		}
		void reset() {
			i2c_cmd_link_delete(h);
			h = i2c_cmd_link_create();
			assert(h);
		}
		void start() {
			ESP_ERROR_THROW(i2c_master_start(h));
		}
		void stop() {
			ESP_ERROR_THROW(i2c_master_stop(h));
		}
		void write(segment seg, bool ack_en = true) {
			ESP_ERROR_THROW(i2c_master_write(h, seg.data, seg.size, ack_en));
		}
		void write(uint8_t byte, bool ack_en = true) {
			ESP_ERROR_THROW(i2c_master_write_byte(h, byte, ack_en));
		}
		void read(segment seg, i2c_ack_type_t ack = I2C_MASTER_LAST_NACK) {
			ESP_ERROR_THROW(i2c_master_read(h, seg.data, seg.size, ack));
		}
		esp_err_t begin(i2c_port_t port = I2C_NUM_0, TickType_t ticks_to_wait = pdMS_TO_TICKS(10)) {
			return i2c_master_cmd_begin(port, h, ticks_to_wait);
		}
	};
	class dev {
	public:
		static constexpr const char *TAG = "i2c::dev";
		uint8_t addr;
		void write_reg(uint8_t reg, uint8_t arg) {
			I2C::cmd cmd;
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_WRITE);
			cmd.write(reg);
			cmd.write(arg);
//			for (int i = 0; i < sizeof(arg); ++i) {
//				cmd.write(((uint8_t*)&arg)[sizeof(arg) - 1 - i]);
//			}
			cmd.stop();
			auto rc = cmd.begin(I2C_NUM_0);
			if (rc != ESP_OK) {
				ESP_LOGE(TAG, "write_reg failed, addr = %d; reg = %d; rc = %s", addr, reg, esp_err_to_name(rc));
			}
		}
//		void write_reg(uint8_t reg) {
//			I2C::cmd cmd;
//			cmd.start();
//			cmd.write(addr << 1 | I2C_MASTER_WRITE);
//			cmd.write(reg);
//			cmd.stop();
//			auto rc = cmd.begin(I2C_NUM_0);
//			if (rc != ESP_OK) {
//				ESP_LOGE(TAG, "write_reg failed, addr = %d; reg = %d; rc = %s", addr, reg, esp_err_to_name(rc));
//			}
//		}
		template <class C> C read_reg(uint8_t reg) {
			I2C::cmd cmd;
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_WRITE);
			cmd.write(reg);
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_READ);
			C res = 0;
			segment seg{
				(uint8_t*)&res, sizeof(C)
			};
			cmd.read(seg, I2C_MASTER_LAST_NACK);
			cmd.stop();
			auto rc = cmd.begin(I2C_NUM_0);
			if (rc != ESP_OK) {
				ESP_LOGE(TAG, "read_reg failed, addr = %d; reg = %d; rc = %s", addr, reg, esp_err_to_name(rc));
			}
			for (size_t i = 0; i < sizeof(C) / 2; ++i) {
				std::swap(seg[i], seg[sizeof(C) - 1 - i]);
			}
			return res;
		}
		void read_reg(uint8_t reg, segment seg) {
			I2C::cmd cmd;
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_WRITE);
			cmd.write(reg);
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_READ);
			cmd.read(seg);
			cmd.stop();
			auto rc = cmd.begin(I2C_NUM_0);
			if (rc != ESP_OK) {
				ESP_LOGE(TAG, "read_reg failed, addr = %d; reg = %d; rc = %s", addr, reg, esp_err_to_name(rc));
			}
		}

	};
	void setup();
	void scan();
};
