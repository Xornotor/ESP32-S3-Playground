/*
 * ATTENTION: This library is optimized to a 240x240 ST7789 Display.
 * For other resolutions it may need some tweaks.
 */

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_lcd_types.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <fonts.c>

#define     LCD_HOST    SPI2_HOST

#define     SCLK_PIN    12
#define     MOSI_PIN    11
#define     MISO_PIN    13
#define     RST_PIN     15
#define     DC_PIN      16 
#define     SWAP_AXIS   true
#define     MIRROR_X    false
#define     MIRROR_Y    true

static const spi_bus_config_t buscfg = {
        .sclk_io_num = SCLK_PIN,
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = MISO_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 240 * 240 * sizeof(uint16_t),
};

static esp_lcd_panel_io_handle_t io_handle = NULL;

static const esp_lcd_panel_io_spi_config_t io_config = {
    .dc_gpio_num = DC_PIN,
    .cs_gpio_num = -1,
    .pclk_hz = 25 * 1000 * 1000,
    .spi_mode = 3,
    .trans_queue_depth = 10,
    .lcd_cmd_bits = 8,
    .lcd_param_bits = 8,
};

static esp_lcd_panel_handle_t panel_handle = NULL;

static const esp_lcd_panel_dev_config_t panel_config = {
    .reset_gpio_num = RST_PIN,
    .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
    .bits_per_pixel = 16,
};

esp_err_t st7789_update_frame(uint16_t *f_buffer){
    uint16_t x_start = 0, y_start = 0;
    if(MIRROR_Y){
        if(SWAP_AXIS) x_start = 80;
        else y_start = 80;
    }
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, x_start, y_start, x_start+240, y_start+240, (const void*) f_buffer));
    return ESP_OK;
}

esp_err_t st7789_init() {
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, MIRROR_X, MIRROR_Y));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, SWAP_AXIS));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    return ESP_OK;
}

esp_err_t st7789_teste(uint16_t *f_buffer){
    uint16_t color = 0x0000;
    uint16_t i, j;

    while(true){
        for(i = 0; i < 240; i++){
            for(j = 0; j < 240; j++){
                f_buffer[240*i + j] = color;
            }
            //color += 0x1;
        }
        color += 0xF;
        ESP_ERROR_CHECK(st7789_update_frame(f_buffer));
        vTaskDelay(pdMS_TO_TICKS(33));
        if(color >= 0xFFFF) break;
    }
    return ESP_OK;
}

esp_err_t st7789_color_lines(uint16_t *f_buffer){
    uint16_t color = 0x0000;
    uint16_t i, j;
    for(i = 0; i < 240; i++){
        for(j = 0; j < 240; j++){
            f_buffer[240*i + j] = color;
        }
        color += 0x1;
    }
    return ESP_OK;
}

void st7789_write_char(uint16_t *f_buffer,
              char c, 
              uint16_t x,
              uint16_t y,
              uint16_t font_color,
              uint8_t font_size
              ){

    uint16_t width, height, line;
    uint8_t i, j;
    uint16_t char_lines[26];

    switch(font_size){
        case 1:
            width = 7;
            height = 9;
            for(i = 0; i < height; i++)
                char_lines[i] = Font7x9[(height*(uint16_t) (c-32)) + i];
            break;
        case 2:
            width = 11;
            height = 18;
            for(i = 0; i < height; i++)
                char_lines[i] = Font11x18[(height*(uint16_t) (c-32)) + i];
            break;
        case 3:
            width = 16;
            height = 26;
            for(i = 0; i < height; i++)
                char_lines[i] = Font16x26[(height*(uint16_t) (c-32)) + i];
            break;
        default:
            width = 6;
            height = 8;
            for(i = 0; i < height; i++)
                char_lines[i] = Font6x8[(height*(uint16_t) (c-32)) + i];
            break; 
    }

    for(i = y; i < y+height; i++){
        line = char_lines[i-y] >> (16-width);
        for(j = x; j < x+width; j++){
            if((line & ((uint16_t) 1 << ((width-1) - (j-x)))) > 0)
                f_buffer[j + (240*i)] = font_color;
        }
    }    
}

void st7789_print(uint16_t *f_buffer,
              const char* string, 
              uint16_t x,
              uint16_t y,
              uint16_t font_color,
              uint8_t font_size
              ){

    uint16_t width, height;

    switch(font_size){
        case 1:
            width = 7; height = 9; break;
        case 2:
            width = 11; height = 18; break;
        case 3:
            width = 16; height = 26; break;
        default:
            width = 6; height = 8; break; 
    }
    
    uint16_t x_pos = x, y_pos = y;
    char c;
    while(*string){
        c = *string;
        if(c == '\n'){
            x_pos = x;
            y_pos += height;
        }else{
            st7789_write_char(f_buffer, c, x_pos, y_pos, font_color, font_size);
            x_pos += width;
        }
        string += sizeof(char);
    }
}

void st7789_draw_box(uint16_t *f_buffer,
              uint16_t x_start,
              uint16_t y_start,
              uint16_t x_end,
              uint16_t y_end,
              uint16_t color
              ){
    
    uint16_t i, j;

    for(i = y_start; i < y_end; i++){
        for(j = x_start; j < x_end; j++){
            f_buffer[j + (240*i)] = color;
        }
    }
}