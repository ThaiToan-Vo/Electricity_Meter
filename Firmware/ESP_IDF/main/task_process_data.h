
#ifndef TASK_PROCESS_DATA_H
#define TASK_PROCESS_DATA_H

    extern TaskHandle_t task_process_handle;


   // variables for avarge voltage
    extern float v_acc;
    extern int   v_cnt;

    // variables for avarge curent
    extern float i_acc;
    extern int   i_cnt;
    
    // variables for avarge power
    extern float p_acc;
    extern int   p_cnt;
    extern uint8_t p_idx;


    // Biến lưu giá trị mới nhất
    extern float v_latest;
    extern float i_latest;
    extern float p_latest;

    typedef struct {
    float v;
    float i;
    float p;
    } oled_data_t;

    void app_task_process_data_init();
    
#endif /* TASK_PROCESS_DATA_H */