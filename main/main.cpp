/* Edge Impulse ingestion SDK
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Include ----------------------------------------------------------------- */
#include "driver/gpio.h"
#include "sdkconfig.h"

#include <stdio.h>

#include "ei_device_espressif_esp32.h"

#include "ei_at_handlers.h"
#include "ei_classifier_porting.h"
#include "ei_run_impulse.h"

#include "ei_analogsensor.h"
#include "ei_inertial_sensor.h"
#include "setup.h"
#include "secrets.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

EiDeviceInfo *EiDevInfo = dynamic_cast<EiDeviceInfo *>(EiDeviceESP32::get_device());
static ATServer *at;

/* Private variables ------------------------------------------------------- */

/* Public functions -------------------------------------------------------- */

esp_err_t send_to_ftp(uint8_t *img, size_t len, uint16_t x_min, uint16_t x_max, uint16_t y_min, uint16_t y_max)
{
  // char local_filename[0x100];
  // snprintf(local_filename, sizeof(local_filename), "/sdcard/esp/%04d.jpg", filename_number);
  // timeit("Save to sd card", save_to_sdcard(jpeg, len, local_filename));
	
  char remote_filename[0x100];

	char* temp = getTime();
	// ESP_LOGI(TAG, "GET TIME: %s", temp);
	// getTime();
  snprintf(remote_filename, sizeof(remote_filename), CAMERA_NAME, temp, filename_number);
	// ESP_LOGI(TAG, "After PRINT");

	ESP_LOGI(TAG, "Remote FILENAME: %s", remote_filename);


  NetBuf_t *nData;
  int connection_response = ftpClient->ftpClientAccess(remote_filename, FTP_CLIENT_FILE_WRITE, FTP_CLIENT_BINARY, ftpClientNetBuf, &nData);
		// ESP_LOGI(TAG, "FTP Connection response %i", connection_response);
		
		// ESP_LOGI(TAG, "After connection");

  if (!connection_response)
  {
    ESP_LOGI(TAG, "Could not send file to FTP, reconnecting to FTP...");

    int ftp_err = ftpClient->ftpClientConnect(FTP_HOST, 21, &ftpClientNetBuf);
    ftpClient->ftpClientLogin(FTP_USER, FTP_PASSWORD, ftpClientNetBuf);
    ftpClient->ftpClientChangeDir(FTP_DIR, ftpClientNetBuf);
    connection_response = ftpClient->ftpClientAccess(remote_filename, FTP_CLIENT_FILE_WRITE, FTP_CLIENT_BINARY, ftpClientNetBuf, &nData);
		if(!connection_response) {
			ESP_LOGI(TAG, "No connection response, restarting ESP");
      return ESP_FAIL;
		}
  }
  int write_len = ftpClient->ftpClientWrite(img, len, nData);
  ftpClient->ftpClientClose(nData);

  if (write_len)
  {
    // Success! Blink once and then back to low brightness
    ESP_LOGI(TAG, "SENT TO FTP AS: %s", remote_filename);
    blink(1);
    m5_camera_led_set_brightness(5);
  }
  else
  {
    ESP_LOGI(TAG, "COULD NOT WRITE DATA");
  }

  send_motion_metadata(remote_filename, x_min, x_max, y_min, y_max);

  return ESP_OK;
}

void setup_ftp() {
		// Setup ftp
		ftpClient = getFtpClient();
    int ftp_err = ftpClient->ftpClientConnect(FTP_HOST, 21, &ftpClientNetBuf);

    ESP_LOGI(TAG, "FTP CLIENT %i", ftp_err);

    int t = ftpClient->ftpClientLogin(FTP_USER, FTP_PASSWORD, ftpClientNetBuf);
		ESP_LOGI(TAG, "FTP login %i", t);

    int changedDir = ftpClient->ftpClientChangeDir(FTP_DIR, ftpClientNetBuf);
		if (changedDir == 0) {
			ftpClient->ftpClientMakeDir(FTP_DIR, ftpClientNetBuf);
			ftpClient->ftpClientChangeDir(FTP_DIR, ftpClientNetBuf);
		}
		ESP_LOGI(TAG, "FTP change dir %i", changedDir);

    pref_begin("poach_det", false, NULL);
		// If want to reset counter
    filename_number = pref_getUInt("filename_number", 0);
		if(filename_number > (uint32_t)10) {
			pref_putUInt("filename_number", 0);
		}
    ESP_LOGI(TAG, "filename_number %d", filename_number++);
}
    

extern "C" int app_main()
{


    gpio_pad_select_gpio(GPIO_NUM_21);
    gpio_reset_pin(GPIO_NUM_21);

    gpio_pad_select_gpio(GPIO_NUM_22);
    gpio_reset_pin(GPIO_NUM_22);    
    
    gpio_set_direction(GPIO_NUM_21, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_NUM_22, GPIO_MODE_OUTPUT);    

    /* Initialize Edge Impulse sensors and commands */

    //custom wifi-config set up
    //EiWifiESP32::init();

   // wifi_init_sta();
    //setup_ftp();

    EiDeviceESP32* dev = static_cast<EiDeviceESP32*>(EiDeviceESP32::get_device());

    ei_printf(
        "Hello from Edge Impulse Device SDK.\r\n"
        "Compiled on %s %s\r\n",
        __DATE__,
        __TIME__);

    /* Setup the inertial sensor */
    if (ei_inertial_init() == false) {
        ei_printf("Inertial sensor initialization failed\r\n");
    }

    /* Setup the analog sensor */
    if (ei_analog_sensor_init() == false) {
        ei_printf("ADC sensor initialization failed\r\n");
    }

    at = ei_at_init(dev);
    ei_printf("Type AT+HELP to see a list of commands.\r\n");
    at->print_prompt();

    dev->set_state(eiStateFinished);
    
    while(1){
        /* handle command comming from uart */
        char data = ei_get_serial_byte();

        while (data != 0xFF) {
            at->handle(data);
            data = ei_get_serial_byte();
        }

    }
}