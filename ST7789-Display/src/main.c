#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <xornotor.c>

#define     LCD_HOST    SPI2_HOST

#define     SCLK_PIN    12
#define     MOSI_PIN    11
#define     MISO_PIN    13
#define     RST_PIN     15
#define     DC_PIN      16  
#define     BLK_PIN     7

void app_main() {
    ESP_ERROR_CHECK(gpio_set_direction(BLK_PIN, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_level(BLK_PIN, 1));

    spi_bus_config_t buscfg = {
        .sclk_io_num = SCLK_PIN,
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = MISO_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240 * 40 * sizeof(uint16_t),
    };

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = DC_PIN,
        .cs_gpio_num = -1,
        .pclk_hz = 16 * 1000 * 1000,
        .spi_mode = 3,
        .trans_queue_depth = 10,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
    };
    
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = RST_PIN,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    uint16_t color[10][10];

    int i, j;
    for(i = 0; i < 10; i++){
        for(j = 0; j < 10; j++){
            color[i][j] = 0x0000;
        }
    }

    uint8_t x = 0, y = 0;

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, 240, 240, (const void*) xornotor_map));

    vTaskDelay(pdMS_TO_TICKS(3000));

    while(true){
        //ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, x, y, x+10, y+10, (const void*) color));
        x += 10;
        if (x > 240) {
            x = 0;
            y += 10;
        }
        if (y > 240) {
            x = 0;
            y = 0;
            for(i = 0; i < 10; i++){
                for(j = 0; j < 10; j++){
                    color[i][j] += 0xF;
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}