#include "config.h"
#include "task_read_data.h"
static const char *TAG = "SPI_CONFIG";
spi_device_handle_t spi_v;
spi_device_handle_t spi_i;
esp_err_t ret;

#define INPUT_PIN 25
/*=== SPI bus initialization ===*/

void spi_bus_init(void)
{
    spi_bus_config_t buscfg = {
        .mosi_io_num = 23,
        .miso_io_num = 19,
        .sclk_io_num = 18,
        .max_transfer_sz = 32,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .flags = 0,
    };


    ret = spi_bus_initialize(VSPI_HOST, &buscfg, SPI_DMA_DISABLED);
    if (ret != ESP_OK) 
    {
        ESP_LOGI(TAG, "SPI bus init failed: %d\n", ret);
        return;
    }
    ESP_LOGI(TAG, "SPI bus init success: %d\n", ret);
   
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,
        .queue_size = 2,
        .command_bits = 0,
        .address_bits = 0,
        .dummy_bits = 0,
        .flags = 0,
    };


    devcfg.spics_io_num = 15; // STM voltage
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_v);
    if (ret != ESP_OK) 
    {
        ESP_LOGI(TAG, "Add device V failed: %d\n", ret);
        return;
    }
    //ESP_LOGI(TAG, "Add device V success: %d, handle=%p\n", ret, (void*)spi_v);
   
    devcfg.spics_io_num = 14; // STM current
    ret = spi_bus_add_device(VSPI_HOST, &devcfg, &spi_i);
    if (ret != ESP_OK) 
    {
        ESP_LOGI(TAG, "Add device I failed: %d\n", ret);
        return;
    }
    //ESP_LOGI(TAG, "Add device I success: %d, handle=%p\n", ret, (void*)spi_i);
}



static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if (read_data_handle != NULL)
    {
        vTaskNotifyGiveFromISR(read_data_handle, &xHigherPriorityTaskWoken);
    }

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

/* === External ISR trigger STM32 ===*/
void Ex_ISR_Init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << 13,
        .mode = GPIO_MODE_OUTPUT
    };
    gpio_config(&io);
    gpio_set_level(13, 0);
    gpio_set_level(13, 1);
    //ESP_LOGI(TAG, "ISR triggered sampling");

        // ===== Config GPIO for data ready ISR from STM32 =====//

    gpio_config_t io_conf = {
    .pin_bit_mask = 1ULL << INPUT_PIN,
    .mode = GPIO_MODE_INPUT,
    .intr_type = GPIO_INTR_NEGEDGE,     
    .pull_up_en = GPIO_PULLUP_DISABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(INPUT_PIN, gpio_interrupt_handler, (void *)INPUT_PIN);
    //ESP_LOGI(TAG, "ISR DATA");
}
