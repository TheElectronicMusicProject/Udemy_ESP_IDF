/*
 * http_server.c
 *
 *  Created on: 9 apr 2023
 *      Author: Filippo
 */

#include "http_server.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "tasks_common.h"
#include "wifi_app.h"
#include "esp_ota_ops.h"
#include "sys/param.h"
#include "stdint.h"

static const char g_tag[] = "http_server";

// Task handler
//
static httpd_handle_t g_http_server_handle = NULL;

// HTTP monitor task handler
//
static TaskHandle_t g_task_http_server_monitor = NULL;

// Queue to manipulate the main queue of events
//
static QueueHandle_t g_http_server_queue = NULL;

// Firmware update status
//
static int32_t g_fw_update_status = OTA_UPDATE_PENDING;

static const esp_timer_create_args_t g_fw_update_reset_args = {
	.callback = http_server_fw_update_reset_callback,
	.arg = NULL,
	.dispatch_method = ESP_TIMER_TASK,
	.name = "fw_update_reset"
};

esp_timer_handle_t gh_fw_update_reset = NULL;

// Embedded files: JQuery, html, js, ico, css
//
extern const uint8_t g_jquery_3_3_1_min_js_start[] asm("_binary_jquery_3_3_1_min_js_start");
extern const uint8_t g_jquery_3_3_1_min_js_end[] asm("_binary_jquery_3_3_1_min_js_end");
extern const uint8_t g_index_html_start[] asm("_binary_index_html_start");
extern const uint8_t g_index_html_end[] asm("_binary_index_html_end");
extern const uint8_t g_app_css_start[] asm("_binary_app_css_start");
extern const uint8_t g_app_css_end[] asm("_binary_app_css_end");
extern const uint8_t g_app_js_start[] asm("_binary_app_js_start");
extern const uint8_t g_app_js_end[] asm("_binary_app_js_end");
extern const uint8_t g_favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t g_favicon_ico_end[] asm("_binary_favicon_ico_end");

static httpd_handle_t http_server_configure(void);
static esp_err_t http_server_jquery_handler(httpd_req_t * p_req);
static esp_err_t http_server_index_html_handler(httpd_req_t * p_req);
static esp_err_t http_server_app_css_handler(httpd_req_t * p_req);
static esp_err_t http_server_app_js_handler(httpd_req_t * p_req);
static esp_err_t http_server_favicon_ico_handler(httpd_req_t * p_req);
static esp_err_t http_server_ota_update_handler(httpd_req_t * p_req);
static esp_err_t http_server_ota_status_handler(httpd_req_t * p_req);
static void http_server_monitor(void * p_param);
static void http_server_fw_update_reset_timer(void);

void
http_server_start (void)
{
	if (NULL == g_http_server_handle)
	{
		g_http_server_handle = http_server_configure();
	}
}

void http_server_stop(void)
{
	if (NULL != g_http_server_handle)
	{
		httpd_stop(g_http_server_handle);
		ESP_LOGI(g_tag, "http_server_stop: stopping HTTP server");
		g_http_server_handle = NULL;
	}

	if (NULL != g_task_http_server_monitor)
	{
		vTaskDelete(g_task_http_server_monitor);

		ESP_LOGI(g_tag, "http_server_stop: Stopping HTTP server monitor");
		g_task_http_server_monitor = NULL;
	}
}

void
http_server_fw_update_reset_callback (void * p_arg)
{
	ESP_LOGI(g_tag, "http_server_fw_update_reset_callback: timer timed out, restarting the device");

	esp_restart();
}

BaseType_t
http_server_monitor_send_message (http_server_message_t msg_id)
{
	http_server_queue_message_t msg = {0};

	msg.msg_id = msg_id;

	return xQueueSend(g_http_server_queue, &msg, portMAX_DELAY);
}

static httpd_handle_t
http_server_configure (void)
{
	// Default configuration
	//
	httpd_config_t config = HTTPD_DEFAULT_CONFIG();

	xTaskCreatePinnedToCore(http_server_monitor, "http_server_monitor",
							HTTP_SERVER_MONITOR_STACK_SIZE, NULL,
							HTTP_SERVER_MONITOR_PRIORITY,
							&g_task_http_server_monitor,
							HTTP_SERVER_MONITOR_CORE_ID);

	g_http_server_queue = xQueueCreate(3, sizeof(http_server_queue_message_t));

	// Core of HTTP server
	// task_priority = 1 as default
	// stack_size = 4096 as default
	//
	config.core_id = HTTP_SERVER_TASK_CORE_ID;
	config.task_priority = HTTP_SERVER_TASK_PRIORITY;
	config.stack_size = HTTP_SERVER_TASK_STACK_SIZE;
	config.max_uri_handlers = 20;
	config.recv_wait_timeout = 10;
	config.send_wait_timeout = 10;

	ESP_LOGI(g_tag, "http_server_configure:"
					"Starting server on port: %d"
					"with task priority: %d",
					config.server_port, config.task_priority);

	if (ESP_OK == httpd_start(&g_http_server_handle, &config))
	{
		ESP_LOGI(g_tag, "http_server_configure: Registering the URI handlers");

		httpd_uri_t jquery_js = {
			.uri = "/jquery-3.3.1.min.js",
			.method = HTTP_GET,
			.handler = http_server_jquery_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(g_http_server_handle, &jquery_js);

		httpd_uri_t index_html = {
			.uri = "/",
			.method = HTTP_GET,
			.handler = http_server_index_html_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(g_http_server_handle, &index_html);

		httpd_uri_t app_css = {
			.uri = "/app.css",
			.method = HTTP_GET,
			.handler = http_server_app_css_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(g_http_server_handle, &app_css);

		httpd_uri_t app_js = {
			.uri = "/app.js",
			.method = HTTP_GET,
			.handler = http_server_app_js_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(g_http_server_handle, &app_js);

		httpd_uri_t favicon_ico = {
			.uri = "/favicon.ico",
			.method = HTTP_GET,
			.handler = http_server_favicon_ico_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(g_http_server_handle, &favicon_ico);

		httpd_uri_t ota_update = {
			.uri = "/OTAupdate",
			.method = HTTP_POST,
			.handler = http_server_ota_update_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(g_http_server_handle, &ota_update);

		httpd_uri_t ota_status = {
			.uri = "/OTAstatus",
			.method = HTTP_POST,
			.handler = http_server_ota_status_handler,
			.user_ctx = NULL
		};

		httpd_register_uri_handler(g_http_server_handle, &ota_status);

		return g_http_server_handle;
	}

	return NULL;
}

static esp_err_t
http_server_jquery_handler (httpd_req_t * p_req)
{
	ESP_LOGI(g_tag, "jquery requested");

	httpd_resp_set_type(p_req, "application/javascript");
	httpd_resp_send(p_req, (const char *) g_jquery_3_3_1_min_js_start,
					g_jquery_3_3_1_min_js_end - g_jquery_3_3_1_min_js_start);

	return ESP_OK;
}

static esp_err_t
http_server_index_html_handler (httpd_req_t * p_req)
{
	ESP_LOGI(g_tag, "index.html requested");

	httpd_resp_set_type(p_req, "text/html");
	httpd_resp_send(p_req, (const char *) g_index_html_start,
					g_index_html_end - g_index_html_start);

	return ESP_OK;
}

static esp_err_t
http_server_app_css_handler (httpd_req_t * p_req)
{
	ESP_LOGI(g_tag, "app.css requested");

	httpd_resp_set_type(p_req, "text/css");
	httpd_resp_send(p_req, (const char *) g_app_css_start,
					g_app_css_end - g_app_css_start);

	return ESP_OK;
}

static esp_err_t
http_server_app_js_handler (httpd_req_t * p_req)
{
	ESP_LOGI(g_tag, "app.js requested");

	httpd_resp_set_type(p_req, "application/javascript");
	httpd_resp_send(p_req, (const char *) g_app_js_start,
					g_app_js_end - g_app_js_start);

	return ESP_OK;
}

static esp_err_t
http_server_favicon_ico_handler (httpd_req_t * p_req)
{
	ESP_LOGI(g_tag, "favicon.ico requested");

	httpd_resp_set_type(p_req, "image/x-icon");
	httpd_resp_send(p_req, (const char *) g_favicon_ico_start,
					g_favicon_ico_end - g_favicon_ico_start);

	return ESP_OK;
}

static esp_err_t
http_server_ota_update_handler (httpd_req_t * p_req)
{
	esp_ota_handle_t h_ota = 0;
	char ota_buff[1024] = {0};
	int32_t content_length = p_req->content_len;
	int32_t content_received = 0;
	int32_t recv_len = 0;
	bool b_is_req_body_started = false;
	bool b_flash_successful = false;
	const esp_partition_t * p_update_partition = esp_ota_get_next_update_partition(NULL);

	do
	{
		// Read the data for the request
		//
		if ((recv_len = httpd_req_recv(p_req, ota_buff,
									   MIN(content_length,
									   sizeof(ota_buff)))) < 0)
		{
			if (HTTPD_SOCK_ERR_TIMEOUT == recv_len)
			{
				ESP_LOGI(g_tag, "http_server_ota_update_handler: Socket timeout");

				// Try again
				//
				continue;
			}

			ESP_LOGI(g_tag, "http_server_ota_update_handler: OTA other error %d", recv_len);

			return ESP_FAIL;
		}

		printf("http_server_ota_update_handler: OTA rx %d of %d\n", content_received, content_length);

		// First part received
		//
		if (false == b_is_req_body_started)
		{
			b_is_req_body_started = true;

			char * p_body_start = strstr(ota_buff,
										 OTA_REMOVE_WEB_FORM_DATA) +
										 strlen(OTA_REMOVE_WEB_FORM_DATA);
			int32_t body_part_len = recv_len - (p_body_start - ota_buff);

			printf("http_server_ota_update_handler: OTA file size: %d\n", content_length);

			esp_err_t err = esp_ota_begin(p_update_partition, OTA_SIZE_UNKNOWN, &h_ota);

			if (ESP_OK != err)
			{
				printf("http_server_ota_update_handler: Error %s with OTA begin\n", esp_err_to_name(err));
				return ESP_FAIL;
			}
			else
			{
				printf("http_server_ota_update_handler: writing to partition subtype %d at offset 0x%X\n",
						p_update_partition->subtype, p_update_partition->address);
			}

			// Write this first part of the data
			//
			esp_ota_write(h_ota, p_body_start, body_part_len);

			content_received += body_part_len;
		}
		// Not the first part
		//
		else
		{
			esp_ota_write(h_ota, ota_buff, recv_len);

			content_received += recv_len;
		}
	} while ((recv_len > 0) && (content_received < content_length));

	if (ESP_OK == esp_ota_end(h_ota))
	{
		if (ESP_OK == esp_ota_set_boot_partition(p_update_partition))
		{
			const esp_partition_t * p_boot_partition = esp_ota_get_boot_partition();
			ESP_LOGI(g_tag,
					 "http_server_ota_update_handler: next boot partition subtype %d at offset 0x%X\n",
					 p_boot_partition->subtype, p_boot_partition->address);
			b_flash_successful = true;
		}
		else
		{
			ESP_LOGI(g_tag, "http_server_ota_update_handler: FLASHED ERROR\n");
		}
	}
	else
	{
		ESP_LOGI(g_tag, "http_server_ota_update_handler: esp_ota_end ERROR\n");
	}

	if (true == b_flash_successful)
	{
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_SUCCESSFUL);
	}
	else
	{
		http_server_monitor_send_message(HTTP_MSG_OTA_UPDATE_FAILED);
	}

	return ESP_OK;
}

static esp_err_t
http_server_ota_status_handler (httpd_req_t * p_req)
{
	char ota_json[100] = {0};

	ESP_LOGI(g_tag, "ota_status requested");

	sprintf(ota_json, "{\"ota_update_status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", g_fw_update_status,
					  __TIME__, __DATE__);
	httpd_resp_set_type(p_req, "application/json");
	httpd_resp_send(p_req, ota_json, strlen(ota_json));

	return ESP_OK;
}

static void
http_server_monitor (void * p_param)
{
	http_server_queue_message_t msg = {0};

	for (;;)
	{
		if (pdTRUE == xQueueReceive(g_http_server_queue,
									&msg, portMAX_DELAY))
		{
			switch (msg.msg_id)
			{
				case HTTP_MSG_WIFI_CONNECT_INIT:
					ESP_LOGI(g_tag, "HTTP_MSG_WIFI_CONNECT_INIT");
				break;

				case HTTP_MSG_WIFI_CONNECT_SUCCESS:
					ESP_LOGI(g_tag, "HTTP_MSG_WIFI_CONNECT_SUCCESS");
				break;

				case HTTP_MSG_WIFI_CONNECT_FAIL:
					ESP_LOGI(g_tag, "HTTP_MSG_WIFI_CONNECT_FAIL");
				break;

				case HTTP_MSG_OTA_UPDATE_SUCCESSFUL:
					ESP_LOGI(g_tag, "HTTP_MSG_OTA_UPDATE_SUCCESSFUL");

					g_fw_update_status = OTA_UPDATE_SUCCESSFUL;
					http_server_fw_update_reset_timer();
				break;

				case HTTP_MSG_OTA_UPDATE_FAILED:
					ESP_LOGI(g_tag, "HTTP_MSG_OTA_UPDATE_FAILED");

					g_fw_update_status = OTA_UPDATE_FAILED;
				break;

				default:
				break;
			}
		}
	}
}

static void
http_server_fw_update_reset_timer (void)
{
	if (OTA_UPDATE_SUCCESSFUL == g_fw_update_status)
	{
		ESP_LOGI(g_tag, "http_server_fw_update_reset_timer: FW update successfully");

		ESP_ERROR_CHECK(esp_timer_create(&g_fw_update_reset_args, &gh_fw_update_reset));
		ESP_ERROR_CHECK(esp_timer_start_once(gh_fw_update_reset, 8000000));
	}
	else
	{
		ESP_LOGI(g_tag, "http_server_fw_update_reset_timer: FW update unsuccessful");
	}
}
