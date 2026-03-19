#include <stdio.h>
#include <math.h>
#include <string.h>
#include "driver/spi_master.h"
#include "esp_log.h"
#include "driver/gpio.h"

#ifndef PROCESS_H
#define PROCESS_H
#define FRAME_SAMPLES   50
//#define FRAME_BYTES     (1 + FRAME_SAMPLES * 2)
#define AVG_FRAMES      2
#define ADC_MAX   4095.0f  // 16-bit từ STM
#define VREF      3.3f
#define ADC_TO_VOLT_SCALE (3.3f / 4095.0f)  // Chuyển ADC sang Volt
#define VAC_RMS     220.0f
//#define VAC_PEAK    (VAC_RMS * 1.41421356f)   // 311 V
// Scale: từ Volt ADC sang Volt thực tế (220V RMS = 311V peak)
#define V_SCALE          (311.0f / 1.56f)    // Nếu 1.65V ADC = 311V thực
#define V_OFFSET  1.76f
#define I_ADC_RMS_NOISE  0.028f   // RMS noise khi không có tải (dao động 0.98-1.04V)
#define I_SCALE         28.0f

// #define V_GAIN    (820.0f / 5.6f)
// #define I_OFFSET  1.6f
// #define I_GAIN    (1000.0f / 51.0f)



//============================ Function Prototypes ========================//

// Convert ADC value
float adc_to_vin(uint16_t adc);

// Read sample from SPI slave
bool spi_read_sample(spi_device_handle_t dev, uint16_t *out);

//========== Process voltage =======//

typedef struct {
    float vrms;
} frame_v_t;

frame_v_t process_v_frame(uint16_t *v_buf);

//=================================//

//========== Process current =======//

typedef struct {
    float irms;
    float mean;
    float rms_adc;
} frame_i_t;

frame_i_t process_i_frame(uint16_t *i_buf);

//=================================//

//========== Process power =======//

typedef struct {
    float p;      // công suất tác dụng (W)
} frame_p_t;

frame_p_t process_p_frame(uint16_t *v_buf, uint16_t *i_buf);

//=================================//



#endif /* PROCESS_H */
