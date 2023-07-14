#pragma once
#include "pins.hpp"
#include "driver/twai.h"
#include <deque>

class CAN {
protected:

public:
	static constexpr const char *TAG = "CAN";

	void setup() {
		twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)pins::CAN_D, (gpio_num_t)pins::CAN_R, TWAI_MODE_NORMAL);
		g_config.alerts_enabled = TWAI_ALERT_ERR_PASS | TWAI_ALERT_RX_DATA;
		twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
		twai_filter_config_t f_config = {};
		f_config.acceptance_code = 0x100 << 21;
		f_config.acceptance_mask = ~(0x7FF << 21);
		f_config.single_filter = true;

		ESP_ERROR_CHECK(twai_driver_install(&g_config, &t_config, &f_config));
		ESP_ERROR_CHECK(twai_start());
		ESP_LOGI(TAG, "init done");
	}

	std::deque<std::string> handle_alerts(TickType_t ticks) {
		TimeOut_t xTimeOut;
		vTaskSetTimeOutState(&xTimeOut);
		std::deque<std::string> messages;
		while (xTaskCheckForTimeOut(&xTimeOut, &ticks) != pdTRUE) {
			uint32_t alerts_triggered;
			twai_read_alerts(&alerts_triggered, ticks);
			if (alerts_triggered & TWAI_ALERT_ERR_PASS) {
				ESP_LOGE(TAG, "bus entered passive mode; recovering");
				twai_initiate_recovery();
				messages.emplace_back("bus entered passive mode; recovering");
			}
			twai_message_t msg;
			while (twai_receive(&msg, 0) == ESP_OK) {
				char output_buf[64];
				sprintf(output_buf, "got message: %.*s; id is %08lX", msg.data_length_code, msg.data, msg.identifier);
				messages.emplace_back(output_buf);
			}
		}
		return messages;
	}

	void send() {
		twai_message_t message;
		message.identifier = 0xAAAA;
		message.extd = 1;
		message.data_length_code = 4;
		for (int i = 0; i < 4; i++) {
		    message.data[i] = 0;
		}

		//Queue message for transmission
		if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
		    printf("Message queued for transmission\n");
		} else {
		    printf("Failed to queue message for transmission\n");
		}
	}
};

