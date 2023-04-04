/*
 * rgb_led.c
 *
 *  Created on: 4 apr 2023
 *      Author: Filippo
 */

#include "rgb_led.h"
#include <stdbool.h>
#include <stdint.h>
#include "driver/ledc.h"

static ledc_info_t g_ledc_ch[RGB_LED_CHANNEL_NUM] = {0};

static bool ghb_pwm_init = false;

static void
rgb_led_pwm_init (void)
{
	int32_t rgb_ch = 0;

	g_ledc_ch[0].channel = 		LEDC_CHANNEL_0;
	g_ledc_ch[0].gpio = 		RGB_LED_RED_GPIO;
	g_ledc_ch[0].mode = 		LEDC_HIGH_SPEED_MODE;
	g_ledc_ch[0].timer_index = 	LEDC_TIMER_0;

	g_ledc_ch[1].channel = 		LEDC_CHANNEL_1;
	g_ledc_ch[1].gpio = 		RGB_LED_GREEN_GPIO;
	g_ledc_ch[1].mode = 		LEDC_HIGH_SPEED_MODE;
	g_ledc_ch[1].timer_index = 	LEDC_TIMER_0;

	g_ledc_ch[2].channel = 		LEDC_CHANNEL_2;
	g_ledc_ch[2].gpio = 		RGB_LED_BLUE_GPIO;
	g_ledc_ch[2].mode = 		LEDC_HIGH_SPEED_MODE;
	g_ledc_ch[2].timer_index = 	LEDC_TIMER_0;

	// Configure timer zero
	//
	ledc_timer_config_t ledc_timer =
	{
		.duty_resolution =	LEDC_TIMER_8_BIT,
		.freq_hz = 			100,
		.speed_mode =		LEDC_HIGH_SPEED_MODE,
		.timer_num = 		LEDC_TIMER_0
	};

	ledc_timer_config(&ledc_timer);

	// Configure channels
	//
	for (rgb_ch = 0; rgb_ch < RGB_LED_CHANNEL_NUM; ++rgb_ch)
	{
		ledc_channel_config_t ledc_channel =
		{
			.channel =		g_ledc_ch[rgb_ch].channel,
			.duty = 		0,
			.hpoint =		0,
			.gpio_num = 	g_ledc_ch[rgb_ch].gpio,
			.intr_type =	LEDC_INTR_DISABLE,
			.speed_mode =	g_ledc_ch[rgb_ch].mode,
			.timer_sel = 	g_ledc_ch[rgb_ch].timer_index
		};

		ledc_channel_config(&ledc_channel);
	}

	ghb_pwm_init = true;
}

static void
rgb_led_set_color (uint8_t red, uint8_t green, uint8_t blue)
{
	// Value should be 0 - 255
	//
	ledc_set_duty(g_ledc_ch[0].mode, g_ledc_ch[0].channel, red);
	ledc_update_duty(g_ledc_ch[0].mode, g_ledc_ch[0].channel);

	ledc_set_duty(g_ledc_ch[1].mode, g_ledc_ch[1].channel, green);
	ledc_update_duty(g_ledc_ch[1].mode, g_ledc_ch[1].channel);

	ledc_set_duty(g_ledc_ch[2].mode, g_ledc_ch[2].channel, blue);
	ledc_update_duty(g_ledc_ch[2].mode, g_ledc_ch[2].channel);
}

void
rgb_led_wifi_app_started (void)
{
	if (false == ghb_pwm_init)
	{
		rgb_led_pwm_init();
	}

	rgb_led_set_color(255, 102, 255);
}

// HTTP server started
//
void
rgb_led_http_server_started (void)
{
	if (false == ghb_pwm_init)
	{
		rgb_led_pwm_init();
	}

	rgb_led_set_color(204, 255, 51);
}

// ESP32 is connected to an access point
//
void
rgb_led_wifi_connected (void)
{
	if (false == ghb_pwm_init)
	{
		rgb_led_pwm_init();
	}

	rgb_led_set_color(0, 255, 153);
}

