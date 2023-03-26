#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "GPS.hpp"
#include "nvs_flash.h"
#include "wifi.hpp"
#include "esp_console.h"
#include "led.hpp"
#include "gsm_modem.hpp"
#include "i2c.hpp"
#include "i2c_expander.hpp"
#include "mpu9250.hpp"
#include "argtable3/argtable3.h"
#include "console.hpp"
#include "fs.hpp"
#include "webserver.hpp"
#include "CAN.hpp"
#include "mqtt.hpp"
#include "bluetooth.hpp"
#include "adc.hpp"
#include "server/task.hpp"


GPS gps;
led_strip_handle_t led_strip;
Gsm_modem modem;
I2C i2c;
I2C_expander ex;
MPU9250 mpu;
FS fs;
Webserver ws;
CAN can;
MQTT mq;
BT bt;
ADC adc;
server::CommTask server_task;

extern "C"
void app_main() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    i2c.setup();
//    ex.reset();
    initialise_wifi();
//    bt.setup();
//	fs.setup();
	gps.setup();
	configure_led();
//	modem.setup();
//	mpu.setup();
//	ws.setup();
//	can.setup();
//	mq.setup();
//	adc.setup();
	server_task.start("server_task", 1024*10);

	modem.print_details();
	ESP_LOGI("main", "expander serial number is %04lX", ex.read_uid());

	init_console();

}

/*
 	nvs_handle_t nvs;
	ESP_ERROR_CHECK(nvs_open("app", NVS_READWRITE, &nvs));

	int32_t pos;
	if (nvs_get_i32(nvs, "log_pos", &pos) != ESP_OK) {
		pos = 0;
	} else {
		auto log = FILE_RAII("/fs/log.txt", "w");
		if (log) {
			if (log.tell() < pos) {
				pos = 0;
			}
		} else {
			erase_log(0, 0);
			pos = 0;
		}
	}
	char buf[255];
	while (true) {
		if (pos > 1024*1024) {
    		ESP_LOGW("main", "log is >1M, rewinding");
    		pos = 0;
    		nvs_set_i32(nvs, "log_pos", 0);
		}
		auto can_messages = can.handle_alerts(pdMS_TO_TICKS(10000));
    	try {
    	    auto log = FILE_RAII("/fs/log.txt", "w");
    	    if (!log) {
    	    	for (int i = 0; i < 100; ++i) {
    	    		ESP_LOGE("main", "log corrupted; restarting");
    	    		vTaskDelay(1);
    	    	}
    	    	esp_restart();
    	    }
    	    log.seek(pos);

			gps.dump(buf, [&](){
				log.printf("%s\n", buf);
				mq.enqueue(GPS::TAG, buf);
				if (output_enabled) {
					ESP_LOGI(GPS::TAG, "%s", buf);
				}
			});

			mpu.dump_tilt(buf);
			log.printf("%s\n", buf);
			mq.enqueue(MPU9250::TAG, buf);
			if (output_enabled) {
				ESP_LOGI(MPU9250::TAG, "%s", buf);
			}

			for(auto &s: can_messages) {
				sprintf(buf, "%.*s", s.size(), s.data());
				log.printf("CAN: %s\n", buf);
				mq.enqueue(CAN::TAG, buf);
				if (output_enabled) {
					ESP_LOGI(CAN::TAG, "%s", buf);
				}
			}

			auto res = adc.read();
			sprintf(buf, "0 = %0.2f; 1 = %0.2f; 3 = %0.2f", res[0], res[1], res[2]);
			log.printf("ADC: %s\n", buf);
			if (output_enabled) {
				ESP_LOGI(ADC::TAG, "%s", buf);
			}

			pos = log.tell();
			nvs_set_i32(nvs, "log_pos", pos);
    	} catch (const std::exception &e) {
    		ESP_LOGI("main", "log write exception %s", e.what());
    		erase_log(0, 0);
    	}
    }*/

