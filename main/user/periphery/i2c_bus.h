/*
 * i2c_bus.h
 *
 *  Created on: 21.03.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_I2C_BUS_H_
#define MAIN_USER_PERIPHERY_I2C_BUS_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

extern SemaphoreHandle_t i2c_bus_mutex;

void i2c_bus_mutex_init(void);



#endif /* MAIN_USER_PERIPHERY_I2C_BUS_H_ */
