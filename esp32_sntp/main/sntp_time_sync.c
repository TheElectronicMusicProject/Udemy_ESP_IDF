/*
 * sntp_time_sync.c
 *
 *  Created on: 20 apr 2023
 *      Author: Filippo
 */

#include "sntp_time_sync.h"
#include "esp_log.h"
#include "tasks_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/apps/sntp.h"
#include "http_server.h"
#include "stdbool.h"
#include "wifi_app.h"

static const char g_tag[] = "sntp_time_sync";
static bool gb_sntp_op_mode_set = false;

static void task_sntp_time_sync(void * p_parameter);
static void sntp_time_sync_obtain_time(void);
static void sntp_time_sync_init_sntp(void);

void
sntp_time_sync_task_start (void)
{
	xTaskCreatePinnedToCore(task_sntp_time_sync,
							"task_sntp_time_sync",
							SNTP_TIME_SYNC_TASK_STACK_SIZE,
							NULL,
							SNTP_TIME_SYNC_TASK_PRIORITY,
							NULL,
							SNTP_TIME_SYNC_TASK_CORE_ID);
}

char *
sntp_time_sync_get_time (void)
{
	static char time_buffer[100] = {0};
	time_t now = 0;
	struct tm time_info = {0};

	time(&now);
	localtime_r(&now, &time_info);

	if (time_info.tm_year < (2016 - 1900))
	{
		ESP_LOGI(g_tag, "Time is not set yet");
	}
	else
	{
		strftime(time_buffer, sizeof(time_buffer), "%d.%m.%Y %H:%M:%S",
				 &time_info);
		ESP_LOGI(g_tag, "Current time info %s", time_buffer);
	}

	return time_buffer;
}

static void
sntp_time_sync_init_sntp (void)
{
	ESP_LOGI(g_tag, "init SNTP service");

	if (false == gb_sntp_op_mode_set)
	{
		sntp_setoperatingmode(SNTP_OPMODE_POLL);
		gb_sntp_op_mode_set = true;
	}

	sntp_setservername(0, "pool.ntp.org");
	sntp_init();
	http_server_monitor_send_message(HTTP_MSG_TIME_SERVICE_INITIALIZED);
}

static void
sntp_time_sync_obtain_time (void)
{
	time_t now = 0;
	struct tm time_info = {0};

	time(&now);
	localtime_r(&now, &time_info);

	// Check the time in case we need to init
	//
#if 1
	if (time_info.tm_year < (2016 - 1900))
	{
#else
	static bool b_update = false;

	if (false == b_update)
	{
		b_update = true;
#endif
		sntp_time_sync_init_sntp();

		// Set local time zone
		//
		setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
		tzset();
	}
}

static void
task_sntp_time_sync (void * p_parameter)
{
	for (;;)
	{
		sntp_time_sync_obtain_time();
		vTaskDelay(10000 / portTICK_PERIOD_MS);
	}

	vTaskDelete(NULL);
}
