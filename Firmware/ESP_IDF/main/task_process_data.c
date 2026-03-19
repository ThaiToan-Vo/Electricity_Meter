
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"   
#include "freertos/queue.h"
#include "process.h"
#include "task_process_data.h"
#include "task_read_data.h"
#include "task_oled.h"
#include "esp_log.h"
#include <string.h>


TaskHandle_t task_process_handle;
static const char *TAG = "DATA_PROCESS";

// Buffer để lưu dữ liệu từ queue
uint16_t v_buf[FRAME_SAMPLES];      // Dùng cho RMS
uint16_t vp_buf[FRAME_SAMPLES];     // Dùng cho Power (instant)
uint16_t i_buf[FRAME_SAMPLES];      // Dùng cho RMS
uint16_t ip_buf[FRAME_SAMPLES];     // Dùng cho Power (instant)

// Variables for average voltage
float v_acc = 0.0f;
int   v_cnt = 0;

// Variables for average current
float i_acc = 0.0f;
int   i_cnt = 0;

// Variables for average power
float p_acc = 0.0f;
int   p_cnt = 0 ;

// Biến lưu giá trị mới nhất
float v_latest;
float i_latest;
float p_latest;

void task_process_data(void *pvParameters)
{
    frame_data_t frame;
    
    while(1)
    {
        // Nhận frame từ queue
        if(xQueueReceive(frame_queue, &frame, portMAX_DELAY) == pdTRUE)
        {
            // Copy dữ liệu vào 2 buffer: 1 cho RMS, 1 cho Power
            
            memcpy(v_buf, frame.v_buf, sizeof(v_buf));
            memcpy(vp_buf, frame.v_buf, sizeof(vp_buf));
            memcpy(i_buf, frame.i_buf, sizeof(i_buf));
            memcpy(ip_buf, frame.i_buf, sizeof(ip_buf));
            
        // ===== Process voltage =====
        frame_v_t v = process_v_frame(v_buf);
        v_acc += v.vrms;

        // ===== Process current =====
        frame_i_t i = process_i_frame(i_buf);
        i_acc += i.irms;
        ESP_LOGI(TAG, "Current: %.2f A, Offset: %.2f", i.irms, i.mean);

        // ===== Process power =====
        frame_p_t p = process_p_frame(vp_buf, ip_buf);
        p_acc += p.p;

        v_cnt++;

        // ===== Khi đủ AVG_FRAMES =====
        if (v_cnt >= AVG_FRAMES)
        {
            v_latest = v_acc / AVG_FRAMES;
            i_latest = i_acc / AVG_FRAMES;
            p_latest = p_acc / AVG_FRAMES;
            //ESP_LOGI(TAG, "AVG over %d frames - V: %.2f V, I: %.2f A, P: %.2f W", AVG_FRAMES, v_latest, i_latest, p_latest);
            v_acc = 0.0f;
            i_acc = 0.0f;
            p_acc = 0.0f;
            v_cnt = 0;
            // ===== Gửi snapshot ổn định cho OLED =====
            oled_data_t d = {
                .v = v_latest,
                .i = i_latest,
                .p = p_latest
            };
                xQueueOverwrite(oled_queue, &d);
            }
        
        //UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
        //ESP_LOGI(TAG, "Process Data Task Watermark: %u bytes", watermark);  
        } 
    }
}     

void app_task_process_data_init()
{
    oled_queue = xQueueCreate(1, sizeof(oled_data_t));
    xTaskCreatePinnedToCore(&task_process_data, "task_process_data", 4096, NULL, 4, &task_process_handle, 1);
}
