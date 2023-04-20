/*
 * nvs_app.c
 *
 *  Created on: 18 apr 2023
 *      Author: Filippo
 */
#include "nvs_app.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "wifi_app.h"

static const char g_tag[] = "nvs";
const char g_app_nvs_sta_creds_namespace[] = "stacreds";

esp_err_t
app_nvs_save_sta_creds (void)
{
	nvs_handle h_nvs = 0;
	esp_err_t err = ESP_FAIL;
	ESP_LOGI(g_tag,
			 "app_nvs_save_sta_creds: Saving station mode credentials to flash");
	wifi_config_t * p_wifi_sta_config = wifi_app_get_wifi_config();

	if (NULL != p_wifi_sta_config)
	{
		err = nvs_open(g_app_nvs_sta_creds_namespace, NVS_READWRITE, &h_nvs);

		if (ESP_OK != err)
		{
			printf("app_nvs_save_sta_creds: Error (%s) opening NVS handle!\n",
				   esp_err_to_name(err));
			return err;
		}

		err = nvs_set_blob(h_nvs, "ssid", p_wifi_sta_config->sta.ssid,
						   MAX_SSID_LENGTH);

		// SSID
		//
		if (ESP_OK != err)
		{
			printf("app_nvs_save_sta_creds: Error (%s) setting SSID to NVS!\n",
				   esp_err_to_name(err));
			return err;
		}

		err = nvs_set_blob(h_nvs, "password", p_wifi_sta_config->sta.password,
						   MAX_PASSWORD_LENGTH);

		// Password
		//
		if (ESP_OK != err)
		{
			printf("app_nvs_save_sta_creds: Error (%s) setting password to NVS!\n",
				   esp_err_to_name(err));
			return err;
		}


		err = nvs_commit(h_nvs);

		if (ESP_OK != err)
		{
			printf("app_nvs_save_sta_creds: Error (%s) committing to NVS!\n",
				   esp_err_to_name(err));
			return err;
		}

		nvs_close(h_nvs);

		ESP_LOGI(g_tag, "app_nvs_save_sta_creds: wrote SSID: %s, password: %s",
				 p_wifi_sta_config->sta.ssid, p_wifi_sta_config->sta.password);
	}

	return ESP_OK;
}

bool
app_nvs_load_sta_creds (void)
{
	nvs_handle h_nvs = 0;
	esp_err_t err = ESP_FAIL;

	ESP_LOGI(g_tag,
			 "app_nvs_load_sta_creds: Loading wifi credentials from flash");

	err = nvs_open(g_app_nvs_sta_creds_namespace, NVS_READWRITE, &h_nvs);

	if (ESP_OK != err)
	{
		return false;
	}
	else
	{
		wifi_config_t * p_wifi_sta_config = wifi_app_get_wifi_config();

		if (NULL == p_wifi_sta_config)
		{
			p_wifi_sta_config = (wifi_config_t *) malloc(sizeof(wifi_config_t));
		}

		memset(p_wifi_sta_config, 0, sizeof(wifi_config_t));

		uint32_t wifi_config_size = sizeof(wifi_config_t);
		uint8_t * p_wifi_config_buff = (uint8_t *) malloc(sizeof(uint8_t) * wifi_config_size);
		memset(p_wifi_config_buff, 0, sizeof(wifi_config_t));

		// SSID
		//
		wifi_config_size = sizeof(p_wifi_sta_config->sta.ssid);
		err = nvs_get_blob(h_nvs, "ssid", p_wifi_config_buff, &wifi_config_size);

		if (ESP_OK != err)
		{
			free(p_wifi_config_buff);
			p_wifi_config_buff = NULL;
			printf("app_nvs_load_sta_creds: (%s) no stations SSID found in NVS\n",
				   esp_err_to_name(err));
			return false;
		}

		memcpy(p_wifi_sta_config->sta.ssid, p_wifi_config_buff, wifi_config_size);

		// Password
		//
		wifi_config_size = sizeof(p_wifi_sta_config->sta.password);
		err = nvs_get_blob(h_nvs, "password", p_wifi_config_buff, &wifi_config_size);

		if (ESP_OK != err)
		{
			free(p_wifi_config_buff);
			p_wifi_config_buff = NULL;
			printf("app_nvs_load_sta_creds: (%s) retrieving password\n",
				   esp_err_to_name(err));
			return false;
		}

		memcpy(p_wifi_sta_config->sta.password, p_wifi_config_buff, wifi_config_size);

		free(p_wifi_config_buff);
		p_wifi_config_buff = NULL;

		nvs_close(h_nvs);

		ESP_LOGI(g_tag, "app_nvs_load_sta_creds: SSID: %s, password: %s",
				 p_wifi_sta_config->sta.ssid, p_wifi_sta_config->sta.password);

		return p_wifi_sta_config->sta.ssid[0] != '\0';
	}

	return true;
}

esp_err_t
app_nvs_clear_sta_creds (void)
{
	nvs_handle h_nvs = 0;
	esp_err_t err = ESP_FAIL;

	ESP_LOGI(g_tag,
			 "app_nvs_clear_sta_creds: Clearing wifi station mode credentials");

	err = nvs_open(g_app_nvs_sta_creds_namespace, NVS_READWRITE, &h_nvs);

	if (ESP_OK != err)
	{
		printf("app_nvs_clear_sta_creds: Error (%s) opening NVS handle\n",
			   esp_err_to_name(err));
		return err;
	}

	err = nvs_erase_all(h_nvs);

	if (ESP_OK != err)
	{
		printf("app_nvs_clear_sta_creds: Error (%s) erasing credentials\n",
			   esp_err_to_name(err));
		return err;
	}

	err = nvs_commit(h_nvs);

	if (ESP_OK != err)
	{
		printf("app_nvs_clear_sta_creds: Error (%s) NVS commit\n",
			   esp_err_to_name(err));
		return err;
	}

	nvs_close(h_nvs);
	return ESP_OK;
}
