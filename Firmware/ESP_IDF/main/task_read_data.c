#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <stdio.h>
#include "driver/gpio.h"
#include "config.h"
#include "process.h"
#include "task_read_data.h"
#include "esp_log.h"


static const char *TAG = "READ_DATA";

// Buffer tạm để đọc sample
uint16_t v_buf_raw[FRAME_SAMPLES];
uint16_t i_buf_raw[FRAME_SAMPLES];
uint8_t  v_idx;
uint8_t  i_idx;
uint16_t sample_v;
uint16_t sample_i;

// Queue để gửi frame data
QueueHandle_t frame_queue;

TaskHandle_t read_data_handle;



//==== Read sample from SPI slave when ISR triggered ====//

void task_read_data(void *pvParameters)
{
    //ESP_LOGI(TAG, "task_read_data started, waiting for notification...");
    frame_data_t frame; // struct để gửi qua queue
    while(1)
    {    
        if(ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == 1) 
        {
        /* ===== READ VOLTAGE ===== */
            if (spi_read_sample(spi_v, &sample_v)) 
            {
                if (v_idx < FRAME_SAMPLES) 
                {
                    v_buf_raw[v_idx] = sample_v;
                    ESP_LOGI(TAG, "Read V sample: %d at idx=%d", sample_v, v_idx);
                    v_idx++;
                }
            }

        /* ===== READ CURRENT ===== */
            if (spi_read_sample(spi_i, &sample_i)) 
            {
                if (i_idx < FRAME_SAMPLES) 
                {
                    i_buf_raw[i_idx] = sample_i;
                    ESP_LOGI(TAG, "Read I sample: %d at idx=%d", sample_i, i_idx);
                    i_idx++;
                }
            }

        /* Khi đủ sample, gửi frame qua queue */
            if (v_idx == FRAME_SAMPLES && i_idx == FRAME_SAMPLES)
            {
                
                memcpy(frame.v_buf, v_buf_raw, sizeof(v_buf_raw));
                //ESP_LOGI(TAG, "V buffer: %d", v_buf_raw[0]);
                memcpy(frame.i_buf, i_buf_raw, sizeof(i_buf_raw));
                
                xQueueSend(frame_queue, &frame, portMAX_DELAY);
                
                v_idx = 0;
                i_idx = 0;
            }

            //UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            //ESP_LOGI(TAG, "Read Data Task Watermark: %u bytes", watermark);
            
        }
    }
}

void app_task_read_data_init()
{
    if (frame_queue == NULL) {
        frame_queue = xQueueCreate(6, sizeof(frame_data_t));
        if (frame_queue == NULL) {
            ESP_LOGE(TAG, "Failed to create frame_queue!");
            return;
        }
    }
    xTaskCreatePinnedToCore(&task_read_data, "task_read_data", 4096, NULL, 5, &read_data_handle, 1);
}
