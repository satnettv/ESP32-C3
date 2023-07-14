#pragma once
#include "i2c.hpp"
#include "esp-exception.hpp"

class QMA6100p: protected I2C::dev {
public:

	QMA6100p() {
		addr = 0x12;
	}

	enum addresses: uint8_t {
		addr_chip_id = 0x00,
		addr_data = 0x01,
		addr_na1 = 0x07, // step_cnt
		addr_int_st = 0x09,
		addr_fifo_st = 0x0e,
		addr_fsr = 0x0f,
		addr_bw = 0x10,
		addr_pm = 0x11,
		addr_na2 = 0x12, // step settings
		addr_int_en = 0x16,
		addr_int_map = 0x19,
		addr_step_cfg = 0x1d,
		addr_int_pin_cfg = 0x20,
		addr_int_cfg = 0x21,
		addr_na3 = 0x22,
		addr_os_cust = 0x27,
		addr_tap = 0x2a,
		addr_mot_cfg = 0x2c,
		addr_rst_mot = 0x30,
		addr_fifo_wm = 0x31,
		addr_st = 0x32,
		addr_internal = 0x33,
		addr_na4 = 0x34,
		addr_s_reset = 0x36,
		addr_image = 0x37,
		addr_fifo_cfg = 0x3e,
		addr_fifo_data = 0x3f
	};

	struct reading {
		float x;
		float y;
		float z;
	};

	uint16_t process_buf[32];
	reading data[10];
	std::vector<reading> read_fifo() {
		uint8_t fifo_st = read_reg<uint8_t>((uint8_t)addr_fifo_st);
//		printf("len = %d\n", fifo_st);
		if (fifo_st / 6 > 0) {
			read_reg(addr_fifo_data, {(uint8_t*)process_buf, fifo_st - (fifo_st % (size_t)6)});
			for (size_t i = 0; i < (fifo_st - (fifo_st % (size_t)6)) / 2; ++i) {
				int x;
				if (process_buf[i] & 1 << 15) {
					process_buf[i] -= 1;
					process_buf[i] = ~process_buf[i];
					x = -process_buf[i];
				} else {
					x = process_buf[i];
				}
	//			for (size_t k = 0; k < 16; ++k) {
	//				printf("%d", (process_buf[i] >> (15 - k)) & 1);
	//			}
	//			printf(" ");
				((reinterpret_cast<float*>(data))[i]) = (float)x / 8192. / 4; // 13 бит, 4g
			}
			return std::vector(data, data + (fifo_st / 6));
		}
		return {};
	}

	enum range: uint8_t {
		range_2g = 0b0001,
		range_4g = 0b0010,
		range_8g = 0b0100,
		range_16g = 0b1000,
		range_32g = 0b1111,
	};
	void set_range(range r) {
		write_reg(addr_fsr, r);
	}

	void set_pm(bool on, uint8_t T_RSTB_SINC_SEL = 0, uint8_t MCLK_SEL = 0) {
		write_reg(addr_pm, (uint8_t)(on > 0) << 7 | T_RSTB_SINC_SEL << 4 | MCLK_SEL);
	}

	void set_offsets(uint8_t (&offsets)[3]) {
		write_reg(addr_os_cust, segment{offsets, 3});
	}

	void self_test() {
		write_reg(addr_st, 0b10000000);
	}


	void soft_reset() {
		write_reg(addr_s_reset, 0x86);
		write_reg(addr_s_reset, 0);
	}

	void fifo_enable() {
//		11 FIFO
//		10 STREAM
//		01 FIFO
//		00 BYPASS
		uint8_t regs = 0b01 << 6 // FIFO_MODE
					 | 0b111 // FIFO_EN_XYZ
					 ;
		write_reg(addr_fifo_cfg, regs);
	}

	void setup() {
		soft_reset();
		set_pm(true);
		write_reg(addr_st, 0b10000000);
		vTaskDelay(1);
		set_range(range_4g);
		write_reg(addr_bw, 0b11111111);
//		printf("%02X %02X %02X %02X ", read_reg.)
		fifo_enable();
	}
};

extern QMA6100p accel;
