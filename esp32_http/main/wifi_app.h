/*
 * wifi_app.h
 *
 *  Created on: 8 apr 2023
 *      Author: Filippo
 */

#ifndef MAIN_WIFI_APP_H_
#	define MAIN_WIFI_APP_H_

#	include "esp_netif.h"

#	define WIFI_AP_SSID				"ESP32_AP"
#	define WIFI_AP_PASSWORD			"password"

// Channel for WiFi
//
#	define WIFI_AP_CHANNEL			1

// SSID is visible
//
#	define WIFI_AP_SSID_HIDDEN		0

// Maximum number of connected devices
//
# 	define WIFI_AP_MAX_CONNECTIONS	5

// Beacon broadcast interval in ms (if low, beacon
// is sent more frequently but it responds also more
// quickly) as recommended.
//
#	define WIFI_AP_BEACON			100

#	define WIFI_AP_IP				"192.168.0.1"
#	define WIFI_AP_GATEWAY			"192.168.0.1"
#	define WIFI_AP_NETMASK			"255.255.255.0"

// 20MHz of bandwidth - 72Mbps of data rate
// This bandwidth minimize the channel interference
//
#	define WIFI_AP_BANDWIDTH		WIFI_BW_HT20

// Power save not used
//
#	define WIFI_STA_POWER_SAVE		WIFI_PS_NONE

// IEEE standard maximum
//
#	define MAX_SSID_LENGTH			32

// IEEE standard maximum
//
#	define MAX_PASSWORD_LENGTH		64

// Retry number on disconnect
//
#	define MAX_CONNECTION_RETRIES	5

// Messages for application task
//
typedef enum wifi_app_message
{
	WIFI_APP_MSG_START_HTTP_SERVER = 0,
	WIFI_APP_MSG_CONNECTING_FROM_HTTP_SERVER,
	WIFI_APP_MSG_STA_CONNECTED_GOT_IP
} wifi_app_message_t;

// For message queue
//
typedef struct wifi_app_queue_message
{
	wifi_app_message_t msg_id;
} wifi_app_queue_message_t;

// Sends a message to the queue
//
BaseType_t wifi_app_send_message(wifi_app_message_t msg_id);

// Starts the WiFi
//
void wifi_app_start(void);

#endif /* MAIN_WIFI_APP_H_ */
