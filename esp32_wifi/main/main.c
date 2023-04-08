#include "nvs_flash.h"
#include "wifi_app.h"
#include "esp_err.h"

void
app_main (void)
{
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
}
