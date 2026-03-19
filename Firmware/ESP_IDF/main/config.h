
#include "driver/spi_master.h"
#include "esp_log.h"
#include "driver/gpio.h"


#ifndef CONFIG_H
#define CONFIG_H

extern spi_device_handle_t spi_v;
extern spi_device_handle_t spi_i;
extern esp_err_t ret;

void spi_bus_init(void);
void Ex_ISR_Init(void);

#endif /* CONFIG_H */