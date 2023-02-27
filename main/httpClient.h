#ifndef ESP_HTTP_CLIENT_
#define ESP_HTTP_CLIENT_

#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_wifi.h"
#include <esp_log.h>
#include "string.h"
#include "constants.h"
#include "secrets.h"
#include "m5stack_camera.h"
#include "sleepHandler.h"

static const char *HTTP_TAG = "http-client";

// Old Dev : Endpoints

// // const char* url = "https://ngulia-api.oscarolsson.me:49160/cameras/status\0";
// static esp_http_client_config_t http_client_config = {
// 			.url = "https://ngulia-api.oscarolsson.me:49160/cameras/status\0",
// 			.cert_pem = ca_cert,
// 			.buffer_size_tx = 1024
// 	};

// static esp_http_client_config_t motion_http_client = {
//       .url = "https://ngulia-api.oscarolsson.me:49160/images/motion\0", 
// 			.cert_pem = ca_cert,
// 			.buffer_size_tx = 1024
// 	};


// Production : Endpoints


// static esp_http_client_config_t http_client_config = {
// 			.url = "http://camera-metadata.eprotection.se:49161/cameras/status\0",
// 			.buffer_size_tx = 1024
// 	};

// static esp_http_client_config_t motion_http_client = {
//       .url = "http://camera-metadata.eprotection.se:49161/images/motion\0", 
// 			.buffer_size_tx = 1024
// 	};

static esp_http_client_config_t http_client_config = {
			.url = "18.210.19.202:3001/cameras/status",
			.buffer_size_tx = 1024
	};

static esp_http_client_config_t motion_http_client = {
     		.url = "18.210.19.202:3001/images/motion", 
			.buffer_size_tx = 1024
	};
esp_err_t update_status(wifi_ap_record_t ap,  float bat_percentage, char *msg, float bat_voltage );

void send_motion_metadata(const char* filename, uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max);

#endif

