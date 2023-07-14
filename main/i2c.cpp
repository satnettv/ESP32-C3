#include "i2c.hpp"
#include "pins.hpp"

void I2C::setup() {
	i2c_config_t conf = i2c_config_t();
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = pins::SDA;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = pins::SCL;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.clk_flags = 0;
	conf.master.clk_speed = 400000;
	ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
	ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
//	ESP_ERROR_CHECK(i2c_set_timeout(0, 0x1F));
}

void I2C::scan() {
	for (uint8_t addr = 0x7E; addr <= 0x7E; ++addr) {
		I2C::cmd cmd;
		cmd.start();
		cmd.write(addr << 1 | I2C_MASTER_WRITE);
		cmd.write(4);
		cmd.stop();
		auto res = cmd.begin(I2C_NUM_0, 10);
		if (res == ESP_OK) {
			I2C::cmd cmd1;
			cmd1.start();
			cmd1.write(addr << 1 | I2C_MASTER_READ);
			uint8_t buf;
			cmd1.read({&buf, 1});
			cmd1.stop();
			cmd1.begin(I2C_NUM_0, 10);
			printf("got device at addr %d, data is %d\n", addr, buf);
		} else if (addr == 0x7E) {
			printf("%s\n", esp_err_to_name(res));
		}
		vTaskDelay(1);
	}
	printf("i2c scan done\n");
}
