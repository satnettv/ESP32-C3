#pragma once
#include <freertos/FreeRTOS.h>
//#include <freertos/task.h>

class Timeout {
	TimeOut_t timeout;
public:
	TickType_t ticks_left;
	Timeout(TickType_t tl): ticks_left(tl) {
		vTaskSetTimeOutState(&timeout);
	}
	bool done() {
		return xTaskCheckForTimeOut(&timeout, &ticks_left) != pdFALSE;
	}
	operator TickType_t() {
		return ticks_left;
	}
};
