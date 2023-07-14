#pragma once
#include "esp_err.h"
#include <stdexcept>

class esp_exception: public std::exception {
public:
	const esp_err_t code;
	char message[512];
	esp_exception(esp_err_t code, const char *file, int line, const char *function, const char *expression): code(code) {
		snprintf(message, 512, "error: \"%s\"\n file: \"%s\" line %d\nfunc: %s\nexpression: %s\n",
				esp_err_to_name(code), file, line, function, expression);
	}

	const char*
		what() const _GLIBCXX_TXN_SAFE_DYN _GLIBCXX_USE_NOEXCEPT override {
		return message;
	}
};

#define ESP_ERROR_THROW(x) ({                                                           \
        esp_err_t err_rc_ = (x);                                                        \
        if (unlikely(err_rc_ != ESP_OK)) {                                              \
            throw esp_exception(err_rc_, __FILE__, __LINE__, __PRETTY_FUNCTION__, #x);  \
        }                                                                               \
    })
