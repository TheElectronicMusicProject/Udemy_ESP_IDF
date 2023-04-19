/*
 * nvs_app.h
 *
 *  Created on: 18 apr 2023
 *      Author: Filippo
 */

#ifndef MAIN_NVS_APP_H_
#	define MAIN_NVS_APP_H_

#	include "esp_err.h"
#	include <stdbool.h>

esp_err_t app_nvs_save_sta_creds(void);
bool app_nvs_load_sta_creds(void);
esp_err_t app_nvs_clear_sta_creds(void);

#endif /* MAIN_NVS_APP_H_ */
