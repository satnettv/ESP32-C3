#pragma once
#include "pins.hpp"
#include "cxx_include/esp_modem_dte.hpp"
#include "cxx_include/esp_modem_dce.hpp"
#include "cxx_include/esp_modem_api.hpp"
#include "esp_modem_config.h"


class Gsm_modem {
	std::shared_ptr<esp_modem::DTE> _dte;
	std::unique_ptr<esp_modem::DCE> _dce;

//	static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
//		auto
//
//	}
public:
	esp_netif_t *esp_netif = 0;
	static constexpr const char *TAG = "gsm_modem";
	esp_modem::DTE &dte() {
		return *_dte;
	}
	esp_modem::DCE &dce() {
		return *_dce;
	}

	void setup() {
//		gpio_config_t g_cfg = {};
//		g_cfg.mode = GPIO_MODE_OUTPUT_OD;
//		g_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
////		g_cfg.pin_bit_mask = (uint32_t)1 << pins::GSM_RST;
//		ESP_ERROR_CHECK(gpio_config(&g_cfg));
//		gpio_set_level((gpio_num_t)pins::GSM_RST, 0);
//		vTaskDelay(pdMS_TO_TICKS(200));
//		gpio_set_level((gpio_num_t)pins::GSM_RST, 1);

		vTaskDelay(pdMS_TO_TICKS(5000));

		using namespace esp_modem;
		esp_log_level_set("command_lib", ESP_LOG_VERBOSE);

	    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG("default_apn");
	    esp_netif_config_t ppp_netif_config = ESP_NETIF_DEFAULT_PPP();
	    esp_netif = esp_netif_new(&ppp_netif_config);
	    assert(esp_netif);

	    esp_modem_dte_config_t dte_config = ESP_MODEM_DTE_DEFAULT_CONFIG();
	    dte_config.uart_config.tx_io_num = pins::GSM_R;
	    dte_config.uart_config.rx_io_num = pins::GSM_T;
	    dte_config.uart_config.rts_io_num = -1;
	    dte_config.uart_config.cts_io_num = -1;
	    dte_config.uart_config.flow_control = ESP_MODEM_FLOW_CONTROL_SW;
	    dte_config.uart_config.port_num = UART_NUM_1;
	    dte_config.dte_buffer_size = dte_config.uart_config.rx_buffer_size / 2;
	    dte_config.uart_config.baud_rate = 115200;
	    _dte = create_uart_dte(&dte_config);

	    _dce = create_SIM800_dce(&dce_config, _dte, esp_netif);
	    assert(_dce != nullptr);
//	    data_mode();
	}
	void print_details() {
		using namespace esp_modem;
	    std::string string;
	    auto res = _dce->get_imsi(string);
	    if (res == command_result::OK) {
	    	ESP_LOGI(TAG, "imsi is %.*s", string.size(), string.data());
	    } else {
	    	ESP_LOGW(TAG, "get_imsi: %s", res == command_result::FAIL ? "FAIL" : "TIMEOUT");
	    }
//	    res = _dce->get_imei(string);
//	    if (res == command_result::OK) {
//	    	ESP_LOGI(TAG, "imei is %.*s", string.size(), string.data());
//	    } else {
//	    	ESP_LOGW(TAG, "get_imei: %s", res == command_result::FAIL ? "FAIL" : "TIMEOUT");
//	    }
//	    res = _dce->get_operator_name(string);
//	    if (res == command_result::OK) {
//	    	ESP_LOGI(TAG, "operator name is %.*s", string.size(), string.data());
//	    } else {
//	    	ESP_LOGW(TAG, "get_operator_name: %s", res == command_result::FAIL ? "FAIL" : "TIMEOUT");
//	    }
//	    res = _dce->get_module_name(string);
//	    if (res == command_result::OK) {
//	    	ESP_LOGI(TAG, "module name is %.*s", string.size(), string.data());
//	    } else {
//	    	ESP_LOGW(TAG, "get_module_name: %s", res == command_result::FAIL ? "FAIL" : "TIMEOUT");
//	    }
//
//	    int rssi, ber;
//	    res = _dce->get_signal_quality(rssi, ber);
//		if (res == command_result::OK) {
//			ESP_LOGI(TAG, "rssi = %d; ber = %d", rssi, ber);
//		} else {
//			ESP_LOGW(TAG, "get_signal_quality: %s", res == command_result::FAIL ? "FAIL" : "TIMEOUT");
//		}
//
//		res = _dce->get_network_attachment_state(rssi);
//		if (res == command_result::OK) {
//			if (rssi) {
//				ESP_LOGI(TAG, "network attached");
//			} else {
//				ESP_LOGW(TAG, "network not attached");
//			}
//		} else {
//			ESP_LOGW(TAG, "get_network_attachment_state: %s", res == command_result::FAIL ? "FAIL" : "TIMEOUT");
//		}
////
//	    bool pin_ok;
//	    res = _dce->read_pin(pin_ok);
//	    if (res == command_result::OK) {
//	    	if (pin_ok) {
//	    		ESP_LOGI(TAG, "pin input is not required");
//	    	} else {
//	    		ESP_LOGW(TAG, "pin input is required");
//	    	}
//	    } else {
//	    	ESP_LOGW(TAG, "read_pin: %s", res == command_result::FAIL ? "FAIL" : "TIMEOUT");
//	    }
	}
	void data_mode() {
	    if (_dce->set_mode(esp_modem::modem_mode::DATA_MODE)) {
	    	ESP_LOGI(TAG, "entered data mode");
	    } else {
	    	ESP_LOGE(TAG, "data mode fail");
	    }
	}
};
