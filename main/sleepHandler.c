#include "sleepHandler.h"


time_t now;

static const char *TAG = "ESP_SLEEP_HANDLER";

void time_sync_notification_cb(struct timeval *tv) {
	ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void) {
	 ESP_LOGI(TAG, "Initializing SNTP");
  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
  sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
  sntp_init();
}

void go_to_sleep(const int deep_sleep_sec)
{

  esp_wifi_stop();
  ESP_LOGI(TAG, "Entering deep sleep for %d seconds", deep_sleep_sec);
  esp_deep_sleep(1000000LL * deep_sleep_sec);
}

void sleep_handler_setup()
{
	// ESP_LOGI(TAG, "I AM WAKING UP HELLO");
  time(&now);
  localtime_r(&now, &timeinfo);

  // Is time set? If not, tm_year will be (1970 - 1900).
  //if (timeinfo.tm_year < (2016 - 1900))
  //{
    ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
    initialize_sntp();
    int retry = 0;
    const int retry_count = 15;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count)
    {
      ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
  //}

	// Time zone, change in constants.h
  setenv("TZ", TIME_ZONE, 1);
  tzset();

  localtime_r(&now, &timeinfo);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "11111 The current date/time is: %s", strftime_buf);
}

char timestamp[40];
char* getTime() {

	time(&now);
	localtime_r(&now, &timeinfo);

	// ESP_LOGI(TAG, "HOUR %02d", timeinfo.tm_hour);
	// ESP_LOGI(TAG, "Minute %02d", timeinfo.tm_min);
	// ESP_LOGI(TAG, "second %02d", timeinfo.tm_sec);
	// ESP_LOGI(TAG, "Month %02d", timeinfo.tm_mon + 1);
	// ESP_LOGI(TAG, "Date %02d", timeinfo.tm_mday);

	// ESP_LOGI(TAG, "YEAR %04d", timeinfo.tm_year + 1900);

  snprintf(timestamp, sizeof(timestamp), "%04d-%02d-%02d_%02d-%02d-%02d", timeinfo.tm_year + 1900,
		timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

	ESP_LOGI(TAG, "TIMESTAMP: %s", timestamp);
	return timestamp;

}

void update_time() {
  time(&now);
  localtime_r(&now, &timeinfo);
}

void sleep_handler_update(wifi_ap_record_t ap, float bat_percentage, float bat_voltage)
{
	
  if (timeinfo.tm_hour >= SLEEP_HOUR)
  {
		char update_msg[0x100];
		int hours_to_sleep  = 24 - timeinfo.tm_hour + WAKE_UP_HOUR;	
		snprintf(update_msg, sizeof(update_msg), "Time is %02d, going to sleep for  %02d hours", timeinfo.tm_hour, hours_to_sleep);
		update_status(ap, bat_percentage, update_msg, bat_voltage);
    go_to_sleep(hours_to_sleep * 60 * 60);
  } else if(timeinfo.tm_hour < WAKE_UP_HOUR) {
		char update_msg[0x100];
		int hours_to_sleep = WAKE_UP_HOUR - timeinfo.tm_hour;	

		snprintf(update_msg, sizeof(update_msg), "Woke up at %02d, going to sleep again for %02d hours", timeinfo.tm_hour, hours_to_sleep);
		update_status(ap, bat_percentage, update_msg, bat_voltage);
    go_to_sleep((hours_to_sleep) * 60 * 60);
	}
}
