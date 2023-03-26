#pragma once
#include <freertos/timers.h>
#include <functional>

class TimerSingle {
	std::function<void()> callback;
	static void vTimerCallback(TimerHandle_t th) {
		auto ts = reinterpret_cast<TimerSingle*>(pvTimerGetTimerID(th));
		xTimerDelete(th, portMAX_DELAY);
		ts->callback();
		delete ts;
	}
	TimerSingle(const char* name, uint32_t ms, std::function<void()> callback): callback(callback) {
		TimerHandle_t th = xTimerCreate(name, ms / portTICK_PERIOD_MS, false, this, vTimerCallback);
		xTimerStart(th, portMAX_DELAY);
	}
	TimerSingle(const TimerSingle&) = delete;
public:
	static void create(const char* name, uint32_t ms, std::function<void()> callback) {
		new TimerSingle(name, ms, callback);
	}
};

class Timer {
	std::function<void()> callback;
	static void vTimerCallback(TimerHandle_t th) {
		auto tr = reinterpret_cast<Timer*>(pvTimerGetTimerID(th));
		tr->callback();
	}
	TimerHandle_t th;
	Timer(const Timer&) = delete;
public:
	Timer(const char* name, uint32_t ms, std::function<void()> callback, bool reload = true): callback(callback) {
		ESP_LOGD("Timer", "construct %s", name);
		th = xTimerCreate(name, ms / portTICK_PERIOD_MS, reload, this, vTimerCallback);
	}
	bool start(TickType_t wait = portMAX_DELAY) {
		return xTimerStart(th, wait);
	}
	bool stop(TickType_t wait = portMAX_DELAY) {
		return xTimerStop(th, wait);
	}
	~Timer() {
		ESP_LOGD("Timer", "delete");
		xTimerDelete(th, portMAX_DELAY);
	}
};
