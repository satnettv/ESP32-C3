#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdint.h>
#include <string>
#include <stdexcept>
#include "esp_log.h"

class Task {
    protected:
    TaskHandle_t handle = 0;
    virtual void task() {};
    static void task_(void *pvParameters) {
    	try {
    		((Task*)pvParameters)->task();
    	} catch(const std::exception &e) {
    		ESP_LOGE("Task", "uncaught exception: %s", e.what());
    	}
        ((Task*)pvParameters)->handle = nullptr;
        vTaskDelete(0);
    }
    Task(const Task &other) = delete;
    Task(Task &&other) = delete;
    Task& operator=(const Task &other) = delete;
    Task& operator=(Task &&other) = delete;

    public:
    Task(){}

    void start(const char *task_name, uint32_t stack, uint32_t priority = 0, int core_id = 0) {
    	if (!handle) {
    		xTaskCreatePinnedToCore(task_, task_name, stack, this, priority, &handle, core_id);
    	} else {
    		throw std::runtime_error("starting task multiple times");
    	}
    }

    virtual ~Task() {
    	if (handle) {
    		vTaskDelete(handle);
    		handle = nullptr;
    	}
    }
};
