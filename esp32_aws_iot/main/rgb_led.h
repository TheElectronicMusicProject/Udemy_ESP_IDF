/*
 * rgb_led.h
 *
 *  Created on: 4 apr 2023
 *      Author: Filippo
 */

#ifndef MAIN_RGB_LED_H
#	define MAIN_RGB_LED_H

#	include <stdint.h>

// LED gpios
//
#	define RGB_LED_RED_GPIO			25
#	define RGB_LED_GREEN_GPIO		26
#	define RGB_LED_BLUE_GPIO		27

// Color mix channels
//
#	define RGB_LED_CHANNEL_NUM		3

typedef struct
{
	int32_t channel;
	int32_t gpio;
	int32_t mode;
	int32_t timer_index;
} ledc_info_t;

// WiFi application started
//
void rgb_led_wifi_app_started(void);

// HTTP server started
//
void rgb_led_http_server_started(void);

// ESP32 is connected to an access point
//
void rgb_led_wifi_connected(void);

#endif /* MAIN_RGB_LED_H */
