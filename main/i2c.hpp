#pragma once
#include "driver/i2c.h"
#include "pins.hpp"

class I2C {
public:
	class cmd {
	protected:
		i2c_cmd_handle_t h;
	public:
		cmd() {
			h = i2c_cmd_link_create();
		}
		~cmd() {
			i2c_cmd_link_delete(h);
		}
		void start() {
			ESP_ERROR_CHECK(i2c_master_start(h));
		}
		void stop() {
			ESP_ERROR_CHECK(i2c_master_stop(h));
		}
		void write(segment seg, bool ack_en = true) {
			ESP_ERROR_CHECK(i2c_master_write(h, seg.data, seg.size, ack_en));
		}
		void write(uint8_t byte, bool ack_en = true) {
			ESP_ERROR_CHECK(i2c_master_write_byte(h, byte, ack_en));
		}
		void read(segment seg, i2c_ack_type_t ack = I2C_MASTER_LAST_NACK) {
			ESP_ERROR_CHECK(i2c_master_read(h, seg.data, seg.size, ack));
		}
		esp_err_t begin(i2c_port_t port = 0, TickType_t ticks_to_wait = portMAX_DELAY) {
			return i2c_master_cmd_begin(port, h, ticks_to_wait);
		}
	};
	class dev {
	public:
		static constexpr const char *TAG = "i2c::dev";
		uint8_t addr;
		template <class C> void write_reg(uint8_t reg, C arg) {
			I2C::cmd cmd;
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_WRITE);
			cmd.write(reg);
			for (int i = 0; i < sizeof(arg); ++i) {
				cmd.write(((uint8_t*)&arg)[sizeof(arg) - 1 - i]);
			}
//			cmd.write({(uint8_t*)&arg, sizeof(arg)});
			cmd.stop();
			auto rc = cmd.begin(0);
			if (rc != ESP_OK) {
				ESP_LOGE(TAG, "write_reg failed, addr = %d; reg = %d; rc = %s", addr, reg, esp_err_to_name(rc));
			}
		}
		void write_reg(uint8_t reg) {
			I2C::cmd cmd;
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_WRITE);
			cmd.write(reg);
			cmd.stop();
			auto rc = cmd.begin(0);
			if (rc != ESP_OK) {
				ESP_LOGE(TAG, "write_reg failed, addr = %d; reg = %d; rc = %s", addr, reg, esp_err_to_name(rc));
			}
		}
		template <class C> C read_reg(uint8_t reg) {
			I2C::cmd cmd;
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_WRITE);
			cmd.write(reg);
			cmd.start();
			cmd.write(addr << 1 | I2C_MASTER_READ);
			C res;
			segment seg{
				(uint8_t*)&res, sizeof(C)
			};
			cmd.read(seg, I2C_MASTER_LAST_NACK);
			cmd.stop();
			auto rc = cmd.begin(0);
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
			auto rc = cmd.begin(0);
			if (rc != ESP_OK) {
				ESP_LOGE(TAG, "read_reg failed, addr = %d; reg = %d; rc = %s", addr, reg, esp_err_to_name(rc));
			}
		}

	};
	void setup() {
		i2c_config_t conf = {};
		conf.mode = I2C_MODE_MASTER;
		conf.sda_io_num = pins::SDA;
		conf.sda_pullup_en = GPIO_PULLUP_DISABLE;
		conf.scl_io_num = pins::SCL;
		conf.scl_pullup_en = GPIO_PULLUP_DISABLE;
		conf.clk_flags = 0;
		conf.master.clk_speed = 100000;
		ESP_ERROR_CHECK(i2c_param_config(0, &conf));
//		ESP_ERROR_CHECK(i2c_set_timeout(0, 800000)); // 100 мс
		ESP_ERROR_CHECK(i2c_driver_install(0, I2C_MODE_MASTER, 0, 0, 0));
	}
};
