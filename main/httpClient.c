#include "httpClient.h"

esp_err_t update_status(wifi_ap_record_t ap, float bat_percentage, char *msg, float bat_voltage ) {
		esp_http_client_handle_t client = esp_http_client_init(&http_client_config);
		esp_http_client_set_method(client, HTTP_METHOD_POST);
		esp_http_client_set_header(client, "Content-Type", "application/json");

		esp_wifi_sta_get_ap_info(&ap);
		// ESP_LOGI(HTTP_TAG, "TEASRAWRA %d", ap.rssi);
		// ESP_LOGI(HTTP_TAG, "TEASRAWRA ssid %s", ap.ssid);
		const char* ssid = (char*) ap.ssid;
		
		cJSON *root;
		root = cJSON_CreateObject();
		
		// const int volt = m5_camera_battery_voltage();
		cJSON_AddItemToObject(root, "time", cJSON_CreateString(getTime()));
		cJSON_AddNumberToObject(root, "voltage", bat_voltage);
		cJSON_AddNumberToObject(root, "wifi", ap.rssi);
		cJSON_AddNumberToObject(root, "percentage", bat_percentage);
		cJSON_AddItemToObject(root, "msg", cJSON_CreateString(msg));
		cJSON_AddItemToObject(root, "camera", cJSON_CreateString(CAMERA_NAME));
		// cJSON_AddStringToObject(root, "ssid", ap.ssid)
		cJSON_AddItemToObject(root, "ssid", cJSON_CreateString(ssid));
		// cJSON_AddNumberToObject(root, "ssid", ap.ssid);

		
		char* postObj = cJSON_Print(root);
		
		ESP_LOGI(HTTP_TAG, "JSON %s", postObj);
		
		esp_http_client_set_post_field(client, postObj, strlen(postObj));
		esp_err_t err = esp_http_client_perform(client);
		
		cJSON_Delete(root);
		if (err == ESP_OK) {
			ESP_LOGI(HTTP_TAG, "SENT TO HTTP OKAY");
		} else {
			ESP_LOGI(HTTP_TAG, "SENT TO HTTP ERROR");
		}
	  // ESP_LOGI(HTTP_TAG, "Battery status voltage: %dmV", m5_camera_battery_voltage());
		esp_http_client_cleanup(client);

    return err;
}

void send_motion_metadata(const char* filename, uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max) {

		esp_http_client_handle_t client = esp_http_client_init(&motion_http_client);

		esp_http_client_set_method(client, HTTP_METHOD_POST);
		esp_http_client_set_header(client, "Content-Type", "application/json");
		
    cJSON *root;
		root = cJSON_CreateObject();
		
		cJSON_AddStringToObject(root, "filename", filename);
		cJSON_AddNumberToObject(root, "y_min", y_min);
		cJSON_AddNumberToObject(root, "x_min", x_min);
		cJSON_AddNumberToObject(root, "y_max", y_max);
		cJSON_AddNumberToObject(root, "x_max", x_max);
		cJSON_AddNumberToObject(root, "height", HEIGHT);
		cJSON_AddNumberToObject(root, "width", WIDTH);

		
		char* postObj = cJSON_Print(root);
		
		ESP_LOGI(HTTP_TAG, "Motion metadata: %s", postObj);
		
		esp_http_client_set_post_field(client, postObj, strlen(postObj));
		esp_err_t err = esp_http_client_perform(client);
		
		cJSON_Delete(root);
		if (err == ESP_OK) {
			ESP_LOGI(HTTP_TAG, "SENT TO HTTP OKAY");
		} else {
			ESP_LOGI(HTTP_TAG, "SENT TO HTTP ERROR");
		}

		esp_http_client_cleanup(client);
}
