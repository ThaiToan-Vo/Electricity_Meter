#include "freertos/queue.h"

#ifndef TASK_OLED_H
#define TASK_OLED_H



extern QueueHandle_t oled_queue;

void app_task_oled_init();
#endif /* TASK_OLED_H */