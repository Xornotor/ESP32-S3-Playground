#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_err.h>
#include <lcd_st7789.c>

uint16_t frame_buffer[240*240];

void app_main() {
    ESP_ERROR_CHECK(st7789_init());
    
    //st7789_color_lines(frame_buffer);    
    st7789_print(frame_buffer, "Isto e um teste", 0, 0, 0xFFFF, 0);
    st7789_print(frame_buffer, "Isto e um teste", 0, 20, 0xFFFF, 1);
    st7789_print(frame_buffer, "Isto e um teste", 0, 40, 0xFFFF, 2);
    st7789_print(frame_buffer, "Isto e um teste", 0, 60, 0xFFFF, 3);

    while(true){
        st7789_update_frame(frame_buffer);
        vTaskDelay(pdMS_TO_TICKS(33));
    }

}