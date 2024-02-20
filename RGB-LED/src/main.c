#include <rgbled.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Entry point de execução
void app_main() {
    init_led();
    while(true){
        color_rainbow();
        vTaskDelay(2);
    };
}