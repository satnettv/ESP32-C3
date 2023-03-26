#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <utility>

template <class C> class Mutex_val;

class Mutex {
protected:
	SemaphoreHandle_t impl;
	struct Lock {
		Mutex &impl;
		bool took;
		Lock(Mutex &impl, TickType_t xTicksToWait = portMAX_DELAY): impl(impl) {
			took = impl.take(xTicksToWait);
		}
		~Lock() {
			if (took) {
				impl.give();
			}
		}
		operator bool() const {
			return took;
		}
	};
public:
	Mutex() {
		impl = xSemaphoreCreateMutex();
	}
	~Mutex() {
		vSemaphoreDelete(impl);
	}
	Lock take_lock(TickType_t xTicksToWait = portMAX_DELAY) {
		return Lock(*this, xTicksToWait);
	}
	bool take(TickType_t xTicksToWait = portMAX_DELAY) {
		return xSemaphoreTake(impl, xTicksToWait);
	}
	bool give() {
		return xSemaphoreGive(impl);
	}
	SemaphoreHandle_t get_impl() {
	    return impl;
	}

	template <class C>
	friend class Mutex_val;
};


template <class C> class Mutex_val {
	Mutex m;
	C c;
public:
	template <typename... Ts> Mutex_val(Ts&&... args): c(std::forward<Ts>(args)...) {}
	class ref {
		Mutex_val<C> &mtx;
		Mutex::Lock l;
		ref(Mutex_val<C> &mtx): mtx(mtx), l(mtx.m.take_lock()) {

		}
		friend Mutex_val;
	public:
		C *operator->() {
			return &mtx.c;
		}
		C &operator*() {
			return mtx.c;
		}
	};
	ref get() {
		return ref(*this);
	}
	friend ref;
};
