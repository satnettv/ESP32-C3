#pragma once
#include "led_strip.h"
#include "pins.hpp"

extern led_strip_handle_t led_strip;

static void configure_led() {
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = pins::LED;
    strip_config.max_leds = 1;
    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000; // 10MHz

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
    led_strip_refresh(led_strip);
}

inline void led_wifi_on() {
	led_strip_set_pixel(led_strip, 0, 0, 0, 8);
	led_strip_refresh(led_strip);
}

inline void led_wifi_off() {
	led_strip_set_pixel(led_strip, 0, 0, 0, 0);
	led_strip_refresh(led_strip);
}
