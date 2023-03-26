#pragma once
#include "Timer.hpp"
#include <map>

class RTOS_stats {
public:
	static constexpr float filter_alpha = 0.1;
	struct Task_record {
		std::string name;
		configSTACK_DEPTH_TYPE stack_left;
		BaseType_t core_id;
		uint32_t last_runtime;
		float load = 0;
		float load_avg = 0;
		uint32_t generation = 0;
		Task_record(const TaskStatus_t &t, uint32_t generation) {
			name = t.pcTaskName;
			stack_left = t.usStackHighWaterMark;
			core_id = t.xCoreID;
			last_runtime = t.ulRunTimeCounter;
			load = 0;
			load_avg = 0;
			this->generation = generation;
		}
	};
	std::map<UBaseType_t, Task_record> tasks;
	uint32_t last_runtime = 0;

	Timer t;
	RTOS_stats(): t("RTOS_stats", 10000, [this](){this->timer();}) {
		t.start();
	}
	void timer() {
		auto num = uxTaskGetNumberOfTasks();
		std::vector<TaskStatus_t> st(num);
		uint32_t rt;
		uxTaskGetSystemState(st.data(), num, &rt);
		std::swap(last_runtime, rt);
		for(TaskStatus_t &t: st) {
			auto it = tasks.find(t.xTaskNumber);
			if (it == tasks.end()) {
				tasks.emplace(t.xTaskNumber, Task_record(t, last_runtime));
			} else {
				auto &task = it->second;
				task.load = (float)(t.ulRunTimeCounter - task.last_runtime) / (float)(last_runtime - rt);
				task.last_runtime = t.ulRunTimeCounter;
				task.load_avg = task.load * filter_alpha + task.load_avg * (1 - filter_alpha);
				task.stack_left = t.usStackHighWaterMark;
				task.generation = last_runtime;
			}
		}
		for (auto it = tasks.begin(); it != tasks.end(); ) {
			if (it->second.generation != last_runtime) {
				it = tasks.erase(it);
			} else {
				++it;
			}
		}
	}
};

