#pragma once
#include "wifi.hpp"
#include "freq_meter.hpp"
#include "fs.hpp"
#include "gsm_modem.hpp"

extern FS fs;
extern Gsm_modem modem;
extern esp_netif_t *sta_netif;
extern I2C_expander ex;

namespace {
	struct {
		struct arg_end *end = arg_end(0);
	} no_args;


	volatile bool output_enabled = false;
	int toggle_output(int argc, char **argv) {
		if (output_enabled) {
			ESP_LOGI("main", "log output disabled");
			output_enabled = false;
		} else {
			ESP_LOGI("main", "log output enabled");
			output_enabled = true;
		}
		return 0;
	}
	Freq_meter *f_meter;
	int toggle_f_meter(int argc, char **argv) {
		if (f_meter) {
			delete f_meter;
			f_meter = nullptr;
		} else {
			f_meter = new Freq_meter;
		}
		return 0;
	}

	int print_status(int argc, char **argv) {
		modem.print_details();
		ESP_LOGI("main", "%luk ram free", esp_get_free_internal_heap_size() / 1024);
		ESP_LOGI("main", "wifi is %s", esp_netif_is_netif_up(sta_netif) ? "up" : "down");
		ESP_LOGI("main", "ppp is %s", esp_netif_is_netif_up(modem.esp_netif) ? "up" : "down");
		return 0;
	}

	int reset(int argc, char **argv) {
		esp_restart();
		return 0;
	}

	int erase_log(int argc, char **argv) {
		ESP_LOGW("main", "erasing log");
		auto it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, "fs");
		auto part = esp_partition_get(it);
		ESP_ERROR_CHECK(esp_partition_erase_range(part, 0, part->size));
		esp_restart();
		return 0;
	}

	struct {
	    struct arg_str *cmd = arg_str1(NULL, NULL, NULL, "command");
		struct arg_end *end = arg_end(1);
	} at_args;
	int at(int argc, char **argv) {
	    int nerrors = arg_parse(argc, argv, (void **) &at_args);
	    if (nerrors != 0) {
	        arg_print_errors(stderr, at_args.end, argv[0]);
	        return 1;
	    }

		ESP_LOGI("main", "sending modem command %s", at_args.cmd->sval[0]);
		std::string out;
		auto res = modem.dce().at(at_args.cmd->sval[0], out, 15000);
		if (res == esp_modem::command_result::OK) {
			ESP_LOGI("main", "response is %.*s", out.size(), out.data());
		} else {
			ESP_LOGE("main", "%s", res == esp_modem::command_result::FAIL ? "FAIL" : "TIMEOUT");
		}
		return 0;
	}

	struct {
	    struct arg_int *pin = arg_int1(NULL, NULL, NULL, "expander gpio num (0-8)");
		struct arg_end *end = arg_end(1);
	} expander_read_args;
	int expander_read(int argc, char **argv) {
	    int nerrors = arg_parse(argc, argv, (void **)&expander_read_args);
	    if (nerrors != 0) {
	        arg_print_errors(stderr, expander_read_args.end, argv[0]);
	        return 1;
	    }
	    auto &pin = expander_read_args.pin->ival[0];
    	if (pin < 0 || pin > 9) {
    		ESP_LOGE("main", "gpio num is out of range");
    		return 1;
    	}
	    ex.port_mode_input(1 << pin);
	    auto res = ex.digital_read();
	    ESP_LOGI("main", "expander pin %d value is %s", pin, ((1 << pin) & res) ? "high" : "low");
		return 0;
	}

	struct {
	    struct arg_int *pin = arg_int1(NULL, NULL, NULL, "expander gpio num (0-8)");
	    struct arg_int *value = arg_int1(NULL, NULL, NULL, "value (1/0)");
		struct arg_end *end = arg_end(1);
	} expander_write_args;
	int expander_write(int argc, char **argv) {
	    int nerrors = arg_parse(argc, argv, (void **)&expander_write_args);
	    if (nerrors != 0) {
	        arg_print_errors(stderr, expander_write_args.end, argv[0]);
	        return 1;
	    }
	    uint16_t pin = expander_write_args.pin->ival[0];
    	if (pin < 0 || pin > 9) {
    		ESP_LOGE("main", "gpio num is out of range");
    		return 1;
    	}
    	auto &value = expander_write_args.value->ival[0];
    	if (value < 0 || value > 1) {
    		ESP_LOGE("main", "write value is out of range");
    		return 1;
    	}
	    ex.port_mode_output(1u << pin);
	    if (value) {
	    	ex.digital_write_high(1u << pin);
	    } else {
	    	ex.digital_write_low(1u << pin);
	    }
	    ESP_LOGI("main", "expander pin %d value is %s", pin, value ? "high" : "low");
		return 0;
	}
}


void init_console() {
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = ">>";
    repl_config.max_cmdline_length = 128;
    esp_console_register_help_command();
    register_wifi_command();

	esp_console_cmd_t cmd = {
		.command = "log_output",
		.help = "enable_disable navigational data output",
		.hint = NULL,
		.func = &toggle_output,
		.argtable = &no_args
	};
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

	cmd.command = "freq_meter";
	cmd.help = "enable/disable frequency meter on pin 10";
	cmd.func = &toggle_f_meter;
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

	cmd.command = "status";
	cmd.help = "show system status";
	cmd.func = &print_status;
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

	cmd.command = "erase_log";
	cmd.help = "erase log";
	cmd.func = &erase_log;
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

	cmd.command = "reset";
	cmd.help = "reset";
	cmd.func = &reset;
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));


	cmd.command = "expander_read";
	cmd.help = "read expander pin";
	cmd.func = &expander_read;
	cmd.argtable = &expander_read_args;
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

	cmd.command = "expander_write";
	cmd.help = "write to expander pin";
	cmd.func = &expander_write;
	cmd.argtable = &expander_write_args;
	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

//	cmd.command = "at";
//	cmd.help = "modem command";
//	cmd.func = &at;
//	cmd.argtable = &at_args;
//	ESP_ERROR_CHECK(esp_console_cmd_register(&cmd));

    esp_console_dev_usb_serial_jtag_config_t hw_config = ESP_CONSOLE_DEV_USB_SERIAL_JTAG_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_usb_serial_jtag(&hw_config, &repl_config, &repl));
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
