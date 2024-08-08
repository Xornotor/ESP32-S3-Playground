#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_err.h>
#include <lcd_st7789.c>

uint16_t frame_buffer[240*240];

void app_main() {
    uint16_t color = 0x0000;
    ESP_ERROR_CHECK(st7789_init());
    
    //st7789_color_lines(frame_buffer);  
    st7789_draw_box(frame_buffer, 0, 0, 240, 240, 0xFFFF);
    st7789_draw_box(frame_buffer, 10, 10, 230, 30, 0xAAFF);
    st7789_print(frame_buffer, "Interface Teste", 35, 12, 0x0000, 2);
    st7789_print(frame_buffer, "Eu escrevo aqui.\nPorque?\nPORQUE EU QUERO.", 35, 50, 0x1111, 2);

    st7789_print(frame_buffer, "ABLUBLE", 35, 130, 0x0000, 2);

    st7789_draw_box(frame_buffer, 50, 170, 190, 210, 0x4a49);
    st7789_draw_box(frame_buffer, 55, 175, 185, 205, 0x8c51);

    st7789_print(frame_buffer, "BUTAO", 77, 178, 0xFFFF, 3);

    while(true){
        color += 0x0F;
        st7789_draw_box(frame_buffer, 50, 170, 190, 210, color);
        st7789_draw_box(frame_buffer, 55, 175, 185, 205, color+0x1111);

        st7789_print(frame_buffer, "BUTAO", 77, 178, 0xFFFF, 3);
        vTaskDelay(pdMS_TO_TICKS(16));
        st7789_update_frame(frame_buffer);
        vTaskDelay(pdMS_TO_TICKS(17));
    }

}