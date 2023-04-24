/*
 * sntp_time_sync.h
 *
 *  Created on: 20 apr 2023
 *      Author: Filippo
 */

#ifndef MAIN_SNTP_TIME_SYNC_H_
#	define MAIN_SNTP_TIME_SYNC_H_

void sntp_time_sync_task_start(void);
char * sntp_time_sync_get_time(void);

#endif /* MAIN_SNTP_TIME_SYNC_H_ */
