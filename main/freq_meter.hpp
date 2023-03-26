#pragma once
#include "pins.hpp"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"


class Freq_meter {
protected:
	int64_t last_update;
	volatile uint32_t cnt = 0;
	TaskHandle_t handle = nullptr;
public:
	static constexpr const char *TAG = "freq_meter";
	static void IRAM_ATTR isr(void *_arg) noexcept {
		auto arg = (Freq_meter*)_arg;
		arg->cnt = arg->cnt + 1;
	}

	static void task(void *arg) {
		auto fm = (Freq_meter*)arg;
		while(true) {
			auto bits = ulTaskNotifyTake(false, pdMS_TO_TICKS(3000));
			if (bits) {
				vTaskDelete(nullptr);
			} else {
				ESP_LOGI(TAG, "freq = %.0fHz", fm->get_freq());
			}
		}
	}
	Freq_meter() {
		gpio_config_t cfg = {};
		cfg.pin_bit_mask = (uint64_t)1 << pins::INPUT1;
		cfg.mode = GPIO_MODE_INPUT;
		cfg.intr_type = GPIO_INTR_POSEDGE;
		ESP_ERROR_CHECK(gpio_config(&cfg));
		ESP_ERROR_CHECK(gpio_intr_enable((gpio_num_t)pins::INPUT1));
		ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
		ESP_ERROR_CHECK(gpio_isr_handler_add((gpio_num_t)pins::INPUT1, isr, this));
		last_update = esp_timer_get_time();
		xTaskCreate(task, TAG, 2048, this, 0, &handle);
		ESP_LOGI(TAG, "started");
	}
	float get_freq() {
		portDISABLE_INTERRUPTS();
		auto t = esp_timer_get_time();
		auto cnt1 = cnt;
		cnt = 0;
		portENABLE_INTERRUPTS();
		auto t_delta = t - last_update;
		last_update = t;
		float out = cnt1;
		out /= t_delta;
		out *= 1000000;
		return out;
	}
	~Freq_meter() {
		xTaskNotifyGive(handle);
		vTaskDelay(10);
		gpio_uninstall_isr_service();
		gpio_reset_pin((gpio_num_t)pins::INPUT1);
		ESP_LOGI(TAG, "shut down");
	}
};
