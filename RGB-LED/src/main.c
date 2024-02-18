#include <stdio.h>
#include <math.h>
#include <rom/ets_sys.h>
#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint8_t color_weight = 24;
unsigned int color = 0x000000;
const uint32_t freq_hz = 50000;
const uint8_t duty_resolution = 8;
const double duty_unit = (1e9/(freq_hz * 256));
const uint32_t LOW_DCYCLE = (uint32_t)(320/duty_unit);
const uint32_t HIGH_DCYCLE = (uint32_t)(800/duty_unit);

static bool change_color(gptimer_handle_t, const gptimer_alarm_event_data_t*, void*);
void init_color_timer();
void set_color(uint32_t);

const ledc_timer_config_t led_timer_config = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = duty_resolution,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = freq_hz,
    .clk_cfg = LEDC_AUTO_CLK
};

const ledc_channel_config_t led_channel_config = {
    .gpio_num = 48,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = LOW_DCYCLE,
    .hpoint = 0
};

gptimer_handle_t color_timer_handler = NULL;

const gptimer_config_t color_timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = freq_hz
};

const gptimer_alarm_config_t color_alarm_config = {
    .reload_count = 0,
    .alarm_count = 1,
    .flags.auto_reload_on_alarm = true
};

const gptimer_event_callbacks_t color_cbs = {
    .on_alarm = change_color,
};

void init_color_timer(){
    ESP_ERROR_CHECK(gptimer_new_timer(&color_timer_config, &color_timer_handler));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(color_timer_handler, &color_alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(color_timer_handler, &color_cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(color_timer_handler));
}

static bool change_color(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    //printf("Callback\n");
    if(color_weight == 0){
        color_weight = 24;
        ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
        gptimer_stop(timer);
        return true;
    }
    bool bit_val = (bool)((color >> (color_weight-1)) & 1);
    uint32_t color_bit = bit_val ? HIGH_DCYCLE : LOW_DCYCLE;
    //printf("%d", color_bit);
    //ledc_set_duty_and_update(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, color_bit, 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, color_bit);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if(color_weight == 24) ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
    color_weight--;
    return true;
}

//0x000000 to 0xffffff (GRB)
void set_color(uint32_t new_color){
    color = new_color;
    //ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
    //ets_delay_us(100);
    //printf("Antes do gptimer_start\n");
    ESP_ERROR_CHECK(gptimer_start(color_timer_handler));
    //printf("Depois do gptimer_start\n");
    //ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
    //printf("Depois do timer_resume\n");
    //ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 11);
    //ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    //ets_delay_us(160);
    //ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);

}

void app_main() {
    ESP_ERROR_CHECK(ledc_timer_config(&led_timer_config));
    ESP_ERROR_CHECK(ledc_channel_config(&led_channel_config));

    init_color_timer();
    set_color(0x000000);
    color = 1;

    while(true){
        vTaskDelay(10);
        color = color << 1;
        if(color > 0xF00000) color = 1;
        set_color(color);
        printf("%x\n", color);
    };

}