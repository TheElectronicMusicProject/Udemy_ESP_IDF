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
static void http_server_monitor(void * p_param);

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
				break;

				case HTTP_MSG_OTA_UPDATE_FAILED:
					ESP_LOGI(g_tag, "HTTP_MSG_OTA_UPDATE_FAILED");
				break;

				case HTTP_MSG_OTA_UPDATE_INITIALIZED:
					ESP_LOGI(g_tag, "HTTP_MSG_OTA_UPDATE_INITIALIZED");
				break;
			}
		}
	}
}
