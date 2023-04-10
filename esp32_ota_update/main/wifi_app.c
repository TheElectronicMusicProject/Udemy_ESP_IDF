#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "lwip/netdb.h"
#include "rgb_led.h"
#include "tasks_common.h"
#include "wifi_app.h"
#include "http_server.h"

static const char g_tag[] = "wifi_app";

static QueueHandle_t gh_wifi_app_queue = NULL;

esp_netif_t * gp_esp_netif_sta = NULL;
esp_netif_t * gp_esp_netif_ap = NULL;

static void task_wifi_app(void * p_parameter);
static void wifi_app_event_handler_init(void);
static void wifi_app_default_wifi_init(void);
static void wifi_app_soft_ap_config(void);
static void wifi_app_event_handler(void * p_event_handler_arg,
								   esp_event_base_t event_base,
								   int32_t event_id,
								   void * p_event_data);

static void
wifi_app_event_handler (void * p_event_handler_arg,
						esp_event_base_t event_base,
						int32_t event_id,
						void * p_event_data)
{
	if (WIFI_EVENT == event_base)
	{
		switch (event_id)
		{
			case WIFI_EVENT_AP_START:
				ESP_LOGI(g_tag, "WIFI_EVENT_AP_START");
			break;

			case WIFI_EVENT_AP_STOP:
				ESP_LOGI(g_tag, "WIFI_EVENT_AP_STOP");
			break;

			case WIFI_EVENT_AP_STACONNECTED:
				ESP_LOGI(g_tag, "WIFI_EVENT_AP_STACONNECTED");
			break;

			case WIFI_EVENT_AP_STADISCONNECTED:
				ESP_LOGI(g_tag, "WIFI_EVENT_AP_STADISCONNECTED");
			break;

			case WIFI_EVENT_STA_START:
				ESP_LOGI(g_tag, "WIFI_EVENT_STA_START");
			break;

			case WIFI_EVENT_STA_CONNECTED:
				ESP_LOGI(g_tag, "WIFI_EVENT_STA_CONNECTED");
			break;

			case WIFI_EVENT_STA_DISCONNECTED:
				ESP_LOGI(g_tag, "WIFI_EVENT_STA_DISCONNECTED");
			break;

			default:
			break;
		}
	}
	else if (IP_EVENT == event_base)
	{
		switch (event_id)
		{
			case IP_EVENT_STA_GOT_IP:
				ESP_LOGI(g_tag, "IP_EVENT_STA_GOT_IP");
			break;

			default:
			break;
		}
	}
}

static void
wifi_app_default_wifi_init (void)
{
	ESP_ERROR_CHECK(esp_netif_init());
	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	gp_esp_netif_sta = esp_netif_create_default_wifi_sta();
	gp_esp_netif_ap = esp_netif_create_default_wifi_ap();
}

static void
wifi_app_soft_ap_config (void)
{
	wifi_config_t ap_config = {
		.ap = {
			.ssid = WIFI_AP_SSID,
			.ssid_len = strlen(WIFI_AP_SSID),
			.password = WIFI_AP_PASSWORD,
			.channel = WIFI_AP_CHANNEL,
			.ssid_hidden = WIFI_AP_SSID_HIDDEN,
			.authmode = WIFI_AUTH_WPA2_PSK,
			.max_connection = WIFI_AP_MAX_CONNECTIONS,
			.beacon_interval = WIFI_AP_BEACON
		}
	};

	esp_netif_ip_info_t ap_ip_info;
	memset(&ap_ip_info, 0, sizeof(ap_ip_info));

	// Must called first
	//
	esp_netif_dhcps_stop(gp_esp_netif_ap);

	// For numeric binary form
	//
	inet_pton(AF_INET, WIFI_AP_IP, &ap_ip_info.ip);
	inet_pton(AF_INET, WIFI_AP_GATEWAY, &ap_ip_info.gw);
	inet_pton(AF_INET, WIFI_AP_NETMASK, &ap_ip_info.netmask);

	// Statically configure the network interface
	//
	ESP_ERROR_CHECK(esp_netif_set_ip_info(gp_esp_netif_ap, &ap_ip_info));
	ESP_ERROR_CHECK(esp_netif_dhcps_start(gp_esp_netif_ap));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
	ESP_ERROR_CHECK(esp_wifi_set_bandwidth(ESP_IF_WIFI_AP, WIFI_AP_BANDWIDTH));
	ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_STA_POWER_SAVE));
}

static void
wifi_app_event_handler_init (void)
{
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_event_handler_instance_t instance_wifi_event;
	esp_event_handler_instance_t instance_ip_event;

	ESP_ERROR_CHECK(esp_event_handler_instance_register(
			WIFI_EVENT, ESP_EVENT_ANY_ID,
			&wifi_app_event_handler, NULL,
			&instance_wifi_event));
	ESP_ERROR_CHECK(esp_event_handler_instance_register(
			IP_EVENT, ESP_EVENT_ANY_ID,
			&wifi_app_event_handler, NULL,
			&instance_ip_event));
}

static void
task_wifi_app (void * p_parameter)
{
	wifi_app_queue_message_t msg = {0};

	wifi_app_event_handler_init();

	wifi_app_default_wifi_init();

	wifi_app_soft_ap_config();

	ESP_ERROR_CHECK(esp_wifi_start());

	wifi_app_send_message(WIFI_APP_MSG_START_HTTP_SERVER);

	for (;;)
	{
		if (xQueueReceive(gh_wifi_app_queue, &msg, portMAX_DELAY))
		{
			switch (msg.msg_id)
			{
				case WIFI_APP_MSG_START_HTTP_SERVER:
					ESP_LOGI(g_tag, "WIFI_APP_MSG_START_HTTP_SERVER");

					http_server_start();
					rgb_led_http_server_started();
				break;

				case WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER:
					ESP_LOGI(g_tag, "WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER");
				break;

				case WIFI_APP_MSG_STA_CONNECTED_GOT_IP:
					ESP_LOGI(g_tag, "WIFI_APP_MSG_STA_CONNECTED_GOT_IP");

					rgb_led_wifi_connected();
				break;

				default:
				break;
			}
		}
	}
}

BaseType_t
wifi_app_send_message (wifi_app_message_t msg_id)
{
	wifi_app_queue_message_t msg = {0};
	msg.msg_id = msg_id;

	return xQueueSend(gh_wifi_app_queue, &msg, portMAX_DELAY);
}

void
wifi_app_start (void)
{
	ESP_LOGI(g_tag, "STARTING WIFI APPLICATION");
	rgb_led_wifi_app_started();

	// Disable loggin messages
	//
	esp_log_level_set("wifi", ESP_LOG_NONE);

	// Message queue
	//
	gh_wifi_app_queue = xQueueCreate(3, sizeof(wifi_app_message_t));

	xTaskCreatePinnedToCore(task_wifi_app, "wifi_app_task",
							WIFI_APP_TASK_STACK_SIZE, NULL,
							WIFI_APP_TASK_PRIORITY, NULL,
							WIFI_APP_CORE_ID);
}
