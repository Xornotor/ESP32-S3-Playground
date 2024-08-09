#include <esp_heap_caps.h>
#include <driver/i2s_std.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

#define SAMPLERATE  48000
#define SECONDS     1
#define BUFFER_SIZE SAMPLERATE * SECONDS * 2

void app_main() {
    int16_t *buffer = malloc(BUFFER_SIZE * sizeof(int16_t));

    i2s_chan_handle_t tx_handle;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLERATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_NUM_20,
            .dout = GPIO_NUM_21,
            .ws = GPIO_NUM_47,
            .din = I2S_GPIO_UNUSED,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            }
        },
    };

    i2s_channel_init_std_mode(tx_handle, &std_cfg);

    i2s_channel_enable(tx_handle);

    int i;
    double phi = 2 * M_PI;
    for(i = 0; i < BUFFER_SIZE/2; i++){
        double sample = pow(2,8) * sin(2.0 * M_PI * 60 + phi);
        int16_t intsample = (int16_t) sample;
        buffer[2*i] = buffer[2*i+1] = intsample;
        phi *= 1.00019;
    }

    while(true){
        
        i2s_channel_write(tx_handle, buffer, sizeof(int16_t)*BUFFER_SIZE, 0, 1000);
        //vTaskDelay(pdMS_TO_TICKS(1));
    }

}