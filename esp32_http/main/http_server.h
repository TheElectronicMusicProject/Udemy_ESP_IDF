/*
 * http_server.h
 *
 *  Created on: 9 apr 2023
 *      Author: Filippo
 */

#ifndef MAIN_HTTP_SERVER_H_
#	define MAIN_HTTP_SERVER_H_

#	include "freertos/FreeRTOS.h"

// Messages for HTTP monitor
//
typedef enum http_server_message
{
	HTTP_MSG_WIFI_CONNECT_INIT = 0,
	HTTP_MSG_WIFI_CONNECT_SUCCESS,
	HTTP_MSG_WIFI_CONNECT_FAIL,
	HTTP_MSG_OTA_UPDATE_SUCCESSFUL,
	HTTP_MSG_OTA_UPDATE_FAILED,
	HTTP_MSG_OTA_UPDATE_INITIALIZED
} http_server_message_t;

// For message queue
//
typedef struct http_server_queue_message
{
	http_server_message_t msg_id;
} http_server_queue_message_t;

BaseType_t http_server_monitor_send_message(http_server_message_t msg_id);
void http_server_start(void);
void http_server_stop(void);

#endif /* MAIN_HTTP_SERVER_H_ */
