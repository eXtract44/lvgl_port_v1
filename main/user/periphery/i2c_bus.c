#include "i2c_bus.h"

SemaphoreHandle_t i2c_bus_mutex = NULL;

void i2c_bus_mutex_init(void) {
    i2c_bus_mutex = xSemaphoreCreateMutex();
}