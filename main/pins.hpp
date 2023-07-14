#pragma once
#include "PCA9557.hpp"

namespace pins {

static constexpr int
//  LED = 8,
//  GSM_RST = 2,
  INPUT1 = 3,

  GPS_TX = 20,
  //GPS_RX = 21,
  SDA = 8,
  SCL = 9,
  GSM_T = -1, // 7
  GSM_R = 2,
  CAN_D = 6,
  CAN_R = 5
  ;

struct exp_pin {
	const uint8_t pin_num;
	enum mode {
		input, output_low, output_high
	};
	exp_pin(uint8_t arg, mode m = input): pin_num(arg) {
		if (m == output_low) {
			expander.set_output(arg, 0);
			expander.set_port_mode(arg, PCA9557::output);
		} else if (m == output_high) {
			expander.set_output(arg, 1);
			expander.set_port_mode(arg, PCA9557::output);
		} else {
			expander.set_port_mode(arg, PCA9557::input);
		}
	}
	operator uint8_t() {
		return pin_num;
	}
};

static inline exp_pin
	EX_GSM_EN(0, exp_pin::output_low),
	EX_CHARGE_EN(1, exp_pin::output_low),
	EX_LED2(2, exp_pin::output_low),
	EX_GNSS_EN(3, exp_pin::output_low),
	EX_OUT_0(4, exp_pin::output_low),
	EX_OUT_1(5, exp_pin::output_low),
	EX_CAN_STB(7, exp_pin::output_low)
  ;

}
