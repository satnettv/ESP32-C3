#pragma once
#include "freertos/freeRTOS.h"
#include "freertos/task.h"

template <class C> class Task {
protected:
	static void _task(void *arg) {
		reinterpret_cast<C*>(arg)->task();
	}
public:
	TaskHandle_t handle = 0;

	void start(C* host, const char *name, uint32_t stack_depth = 2048, UBaseType_t priority = 0) {
		if (!xTaskCreate(_task, "gps", 2048, host, 0, &handle)) {
			throw std::bad_alloc();
		}
	}
};
