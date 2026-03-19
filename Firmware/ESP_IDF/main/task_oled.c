
#include <stdio.h>
#include <esp_log.h>
#include <ssd1306.h>
#include "task_oled.h"
#include "task_process_data.h"

QueueHandle_t oled_queue;

void task_oled( void *pvParameters)
{
    init_ssd1306();
    oled_data_t d;
    char buf[32];   
    while(1)
    {   
        

        if (xQueueReceive(oled_queue, &d, portMAX_DELAY) == pdTRUE)
        {
            ESP_LOGI("OLED", "V: %.2f I: %.2f P: %.2f", d.v, d.i, d.p);
            ssd1306_clear_buffer();
            snprintf(buf, sizeof(buf), "V: %.2f", d.v);
            ssd1306_print_str(28, 3, buf, false);

            snprintf(buf, sizeof(buf), "I: %.2f", d.i);
            ssd1306_print_str(28, 16, buf, false);

            snprintf(buf, sizeof(buf), "P: %.2f", d.p);
            ssd1306_print_str(28, 32, buf, false);

            ssd1306_print_str(28,47,"Power Meter", false);
            ssd1306_display();
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}
void app_task_oled_init()
{
    xTaskCreatePinnedToCore(&task_oled, "task_oled", 4096, NULL, 3, NULL, 1);
}