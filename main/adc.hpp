#pragma once
#include "esp_adc/adc_oneshot.h"

class ADC {
    adc_oneshot_unit_handle_t adc1_handle;
public:
    static constexpr const char *TAG = "ADC";
	void setup() {
	    adc_oneshot_unit_init_cfg_t init_config1 = {
	    		.unit_id = ADC_UNIT_1,
				.ulp_mode = ADC_ULP_MODE_DISABLE
	    };
	    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

	    adc_oneshot_chan_cfg_t config = {
		        .atten = ADC_ATTEN_DB_11,
	    		.bitwidth = ADC_BITWIDTH_DEFAULT,
	    };
	    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_0, &config));
	    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &config));
	    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_3, &config));
	}

	std::array<float,3> read() {
		std::array<float,3> out;
		int res;
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &res));
		out[0] = (float)res / 4095.;
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &res));
		out[1] = (float)res / 4095.;
		ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL_3, &res));
		out[2] = (float)res / 4095.;
		return out;
	}

};

