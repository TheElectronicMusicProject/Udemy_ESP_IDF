/*
 * wifi_reset_button.c
 *
 *  Created on: 19 apr 2023
 *      Author: Filippo
 */
#include "wifi_reset_button.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "tasks_common.h"
#include "wifi_app.h"

static const char g_tag[] = "wifi_reset_button";

SemaphoreHandle_t gh_wifi_reset_semaphore = NULL;

static void task_wifi_reset_button(void * p_param);
static void IRAM_ATTR isr_wifi_reset_button_handler(void * p_arg);

void
wifi_reset_button_config (void)
{
	gh_wifi_reset_semaphore = xSemaphoreCreateBinary();

	// This button already has a pull-up resistor
	//
	gpio_pad_select_gpio(WIFI_RESET_BUTTON);
	gpio_set_direction(WIFI_RESET_BUTTON, GPIO_MODE_INPUT);

	// Interrupt on the negative edge
	//
	gpio_set_intr_type(WIFI_RESET_BUTTON, GPIO_INTR_NEGEDGE);

	xTaskCreatePinnedToCore(task_wifi_reset_button,
							"wifi_reset_button",
							WIFI_RESET_BUTTON_TASK_STACK_SIZE,
							NULL,
							WIFI_RESET_BUTTON_TASK_PRIORITY,
							NULL, WIFI_RESET_BUTTON_TASK_CORE_ID);

	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

	// Attach interrupt service routine
	//
	gpio_isr_handler_add(WIFI_RESET_BUTTON, isr_wifi_reset_button_handler, NULL);
}

static void
task_wifi_reset_button (void * p_param)
{
	for (;;)
	{
		if (pdTRUE == xSemaphoreTake(gh_wifi_reset_semaphore, portMAX_DELAY))
		{
			ESP_LOGI(g_tag, "WIFI RESET BUTTON INTERRUPT OCCURRED");

			wifi_app_send_message(WIFI_APP_MSG_USER_REQUESTED_STA_DISCONNECT);

			vTaskDelay(2000 / portTICK_PERIOD_MS);
		}
	}
}

static void IRAM_ATTR
isr_wifi_reset_button_handler (void * p_arg)
{
	xSemaphoreGiveFromISR(gh_wifi_reset_semaphore, NULL);
}
