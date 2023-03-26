#pragma once
#include "esp_http_server.h"
#include "fs.hpp"

class Webserver {
public:
	static constexpr const char *TAG = "webserver";
protected:
	httpd_handle_t server;

	static esp_err_t output_log(httpd_req_t *req)	{
		httpd_resp_set_type(req, "text/plain");
		FILE_RAII f("/fs/log.txt", "r");
		if (!f) {
			ESP_LOGE(TAG, "fopen failed, error = %s", strerror(errno));
			return httpd_resp_send(req, "empty", -1);
		}
		char buf[512];
		auto size = f.read(buf, sizeof(buf));
		while (size > 0) {
			if (httpd_resp_send_chunk(req, buf, size) != ESP_OK) {
	            httpd_resp_sendstr_chunk(req, NULL);
	            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
	            return ESP_FAIL;
	        }
			size = f.read(buf, sizeof(buf));
		}
	    return httpd_resp_send_chunk(req, NULL, 0);
	}
public:
	void setup() {
		httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
		ESP_ERROR_CHECK(httpd_start(&server, &cfg));
		httpd_uri_t uri = {};
		uri.handler = output_log;
		uri.method = HTTP_GET;
		uri.uri = "/";
		ESP_ERROR_CHECK(httpd_register_uri_handler(server, &uri));
	}
};
