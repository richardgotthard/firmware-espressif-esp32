#ifndef ESP_SLEEP_HANDLER_
#define ESP_SLEEP_HANDLER_

#include "esp_sleep.h"
#include "esp_sntp.h"
#include "esp_log.h"
#include "constants.h"
#include "esp_wifi.h"
#include "httpClient.h"


struct tm timeinfo;


void sleep_handler_setup();
void sleep_handler_update(wifi_ap_record_t ap, float bat_percentage, float bat_voltage);
void go_to_sleep(const int deep_sleep_sec);
char* getTime();
void update_time();

#endif // ESP_SLEEP_HANDLER_
