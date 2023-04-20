#include "nvs_flash.h"
#include "wifi_app.h"
#include "esp_err.h"
#include "freertos/timers.h"
#include "DHT22.h"
#include "wifi_reset_button.h"

void
app_main (void)
{
	TickType_t tick_wakeup = xTaskGetTickCount();
    // Initialize NVS
	//
	esp_err_t ret = nvs_flash_init();

	if ((ESP_ERR_NVS_NO_FREE_PAGES == ret) ||
		(ESP_ERR_NVS_NEW_VERSION_FOUND == ret))
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}

	ESP_ERROR_CHECK(ret);

	wifi_app_start();

	wifi_reset_button_config();

	vTaskDelayUntil(&tick_wakeup, 1000 / portTICK_PERIOD_MS);

	dht22_task_start();
}
