/**
 * Application entry point
 */

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "bme680_sensor.h"

void
app_main (void)
{
	// Start the BME680 task
	//
	BME680_task_start();
}
