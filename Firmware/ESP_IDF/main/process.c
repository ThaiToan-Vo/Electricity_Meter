#include "process.h"

static const char *TAG = "SPI_PROCESS";

// convert ADC value
float adc_to_vin(uint16_t adc)
{
    return (adc * ADC_TO_VOLT_SCALE);
}

/*=== Master read sample from Slave ===*/ 

bool spi_read_sample(spi_device_handle_t dev, uint16_t *out)
{
    if (!dev) 
    {
        ESP_LOGI(TAG, "ERROR: dev is NULL\n");
        return false;
    }

    if (!out) 
    {
        ESP_LOGI(TAG, "ERROR: out is NULL\n");
        return false;
    }
   
    // Use static transaction to avoid stack corruption
    static spi_transaction_t t;
    memset(&t, 0, sizeof(spi_transaction_t));
   
    t.length = 16;
    t.flags = SPI_TRANS_USE_RXDATA | SPI_TRANS_USE_TXDATA;
    t.tx_data[0] = 0;
    t.tx_data[1] = 0;


    esp_err_t ret = spi_device_transmit(dev, &t);
    if (ret != ESP_OK) 
    {
        ESP_LOGI(TAG, "SPI transmit failed: %d\n", ret);
        return false;
    }


    *out = t.rx_data[0]  | (t.rx_data[1] << 8 );
    return true;
}

/*=== Process voltage ===*/

frame_v_t process_v_frame(uint16_t *v_buf)
{
    frame_v_t r;
    float mean = 0.0f;
    float sum  = 0.0f;


    /* 1. ADC -> Vin, tính mean */
    for (int i = 0; i < FRAME_SAMPLES; i++) 
    {
        float vin = (v_buf[i] * VREF) / ADC_MAX;
        mean += vin;
    }
    mean /= FRAME_SAMPLES;



    /* 2. Trừ mean, tính RMS tại ADC */
    for (int i = 0; i < FRAME_SAMPLES; i++) 
    {
        float vin = adc_to_vin(v_buf[i]);
        float v_ac = vin - mean;
        sum += v_ac * v_ac;
    }


    float vin_rms = sqrtf(sum / FRAME_SAMPLES);


    /* 3. Scale RMS - từ Volt tham chiếu sang Volt thực tế */
    r.vrms = vin_rms * V_SCALE;  
   


    return r;
}

/*=== Process current ===*/

frame_i_t process_i_frame(uint16_t *i_buf)
{
    frame_i_t r;
    static float offsetI = 1863.0f; // giá trị ADC với vref=3.36, và adc 12 bit
    float sum  = 0.0f;
    float filter_I = 0.0f;
    float sqI =0.0f; // square current (bình phương dòng điện)


    // for (int i = 0; i < FRAME_SAMPLES; i++) 
    // {
    //     offsetI += adc_to_vin(i_buf[i]);
    // }
    // offsetI /= FRAME_SAMPLES;


    for (int i = 0; i < FRAME_SAMPLES; i++) 
    {
        // float i_ac = adc_to_vin(i_buf[i]) - offsetI;
        // sum += i_ac * i_ac;

        offsetI = (offsetI +(i_buf[i] - offsetI)/1024.0f);
        filter_I = i_buf[i] - offsetI;
        sqI = filter_I * filter_I;
        sum += sqI;
    }


    float rms_adc = sqrtf(sum / FRAME_SAMPLES);


    // /* Trừ noise RMS đúng bản chất */
    // float rms_eff = 0.0f;
    // if (rms_adc > I_ADC_RMS_NOISE) 
    // {
    //     rms_eff = sqrtf( rms_adc * rms_adc - I_ADC_RMS_NOISE * I_ADC_RMS_NOISE );
    // }


    r.irms    = rms_adc * 0.002775f ;  // thay đổi trực tiếp I_scale để kiểm tra 3.53f , 0.002775f là giá trị để biến đổi raw ADC và hệ số 
    r.mean    = offsetI;
    r.rms_adc = rms_adc;


    return r;
}

/*=== Process average power ===*/

frame_p_t process_p_frame(uint16_t *v_buf, uint16_t *i_buf)
{
    frame_p_t r;
    float mean_v = 0.0f;
    float mean_i = 0.0f;
    float sum_p  = 0.0f;


    /* 1. Tính mean */
    for (int n = 0; n < FRAME_SAMPLES; n++) 
    {
        mean_v += adc_to_vin(v_buf[n]);
        mean_i += adc_to_vin(i_buf[n]);
    }
    mean_v /= FRAME_SAMPLES;
    mean_i /= FRAME_SAMPLES;


    /* 2. Trừ DC offset, triệt tiêu noise, rồi nhân tức thời */
    for (int n = 0; n < FRAME_SAMPLES; n++) 
    {
        float v_ac = (adc_to_vin(v_buf[n]) - mean_v)* V_SCALE;  // Chuyển sang Volt thực tế ngay khi tính v_ac
        float i_ac = (adc_to_vin(i_buf[n]) - mean_i)*I_SCALE;
       
        // Triệt tiêu noise từ i_ac trước nhân
        // Công thức: i_ac_cleaned² = i_ac² - noise²
        float i_ac_sq = i_ac * i_ac;
        float noise_sq = I_ADC_RMS_NOISE * I_ADC_RMS_NOISE;
        sum_p += v_ac * i_ac;
       
        // if (i_ac_sq > noise_sq) 
        // {
        //     // Áp dụng noise subtraction, giữ dấu của i_ac
        //     float i_ac_cleaned = (i_ac >= 0 ? 1.0f : -1.0f) * sqrtf(i_ac_sq - noise_sq);
        //     sum_p += v_ac * i_ac_cleaned;
        // }
        // Nếu i_ac² < noise² thì i_ac_cleaned = 0, không cộng vào sum_p
    }


    /* 3. Scale sang Watt */
    // Công suất = (trung bình của v_ac × i_ac) × V_SCALE × I_SCALE
    r.p = (sum_p / FRAME_SAMPLES) ;


    return r;
}