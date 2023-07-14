#pragma once
#include "i2c.hpp"
#include "segment.hpp"

class PCA9557: protected I2C::dev {

public:
	uint8_t regs[4];
	enum addresses: uint8_t {
		addr_input = 0b00,
		addr_output = 0b01,
		addr_invert = 0b10,
		addr_config = 0b11,
	};

	PCA9557() {
		addr = 0b0011100;
		reset();
	}

	void reset() {
		regs[addr_output] = 0;
		regs[addr_invert] = 0b11111111;
		regs[addr_config] = 0b11111111;
	}


	enum port_mode: uint8_t {
		input = 1,
		output = 0
	};
	void set_port_mode(uint8_t pin, port_mode mode) {
		if (mode == input) {
			regs[addr_config] |= 1 << pin;
		} else {
			regs[addr_config] &= ~(1 << pin);
		}
	}

	void set_output(uint8_t pin, uint8_t level) {
		if (level) {
			regs[addr_output] |= 1 << pin;
		} else {
			regs[addr_output] &= ~(1 << pin);
		}
	}

	uint8_t read_pins() {
		return regs[addr_input] = read_reg<uint8_t>(addr_input);
	}

	void send_output() {
		write_reg(addr_output, regs[addr_output]);
	}

	void send_full_config() {
		write_reg(addr_output, segment(regs + addr_output, 3));
	}

	void read_full() {
		read_reg(addr_output, segment(regs, 4));
	}
};


extern PCA9557 expander;
