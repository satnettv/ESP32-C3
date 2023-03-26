#pragma once
#include "segment.hpp"
#include "mqtt_client.h"

static void log_error_if_nonzero(const char *message, int error_code) {
    if (error_code != 0) {
        ESP_LOGE("mqtt", "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI("mqtt", "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI("mqtt", "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI("mqtt", "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI("mqtt", "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        break;
    }
}

class MQTT {
protected:
	esp_mqtt_client_handle_t client;
	char topic_buf[64] = "telematics/";
public:
	void setup() {
		esp_mqtt_client_config_t mqtt_cfg = {};
		mqtt_cfg.broker.address.uri = "mqtt://broker.emqx.io:1883";
		client = esp_mqtt_client_init(&mqtt_cfg);
	    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);
		ESP_ERROR_CHECK(esp_mqtt_client_start(client));
	}

	void enqueue(const char *topic, const char *data) {
		strcpy(topic_buf + strlen("telematics/"), topic);
		esp_mqtt_client_enqueue(client, topic_buf, data, 0, 1, true, true);
	}
};
