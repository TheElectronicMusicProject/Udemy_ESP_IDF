#include "nvs_flash.h"
#include "wifi_app.h"
#include "esp_err.h"
#include "freertos/timers.h"
#include "DHT22.h"
#include "wifi_reset_button.h"
#include "esp_log.h"
#include "sntp_time_sync.h"

static const char g_tag[] = "main";

int aws_iot_demo_main(int argc, char ** argv);

static void wifi_application_connected_events(void);

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

	wifi_app_set_callback(wifi_application_connected_events);
}

static void
wifi_application_connected_events (void)
{
	ESP_LOGI(g_tag, "WiFi application conneted!");
	sntp_time_sync_task_start();
	aws_iot_demo_main(0, NULL);
}
