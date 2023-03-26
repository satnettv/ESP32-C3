#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

class BinarySemphr {
protected:
	SemaphoreHandle_t impl;
	StaticSemaphore_t buf;
public:
	BinarySemphr() {
		impl = xSemaphoreCreateBinaryStatic(&buf);
	}
	~BinarySemphr() {
		vSemaphoreDelete(impl);
	}
	bool take(TickType_t xTicksToWait = portMAX_DELAY) {
		return xSemaphoreTake(impl, xTicksToWait);
	}
	bool give() {
		return xSemaphoreGive(impl);
	}
};
