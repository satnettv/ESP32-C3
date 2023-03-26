#pragma once
#include <pthread.h>
#include "esp-exception.hpp"


template<class T> struct pthread_key {
	pthread_key_t key;
	operator pthread_key_t() {
		return key;
	}
	pthread_key() {
		ESP_ERROR_THROW(pthread_key_create(&key, nullptr));
	}
	T *get() {
		return reinterpret_cast<T*>(pthread_getspecific(key));
	}
	template <class C> void set(C* arg) {
		pthread_setspecific(key, static_cast<T*>(arg));
	}
};
