#include <stdio.h>
#include <math.h>
#include <rom/ets_sys.h>
#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

uint8_t color_weight = 74;
unsigned int color = 0x000000;
uint32_t color_duties[75];
const uint32_t freq_hz = 40000;
const uint8_t duty_resolution = 7;
const double duty_unit = (1e9/(freq_hz * 128));
const uint32_t LOW_DCYCLE = (uint32_t)(300/duty_unit);
const uint32_t HIGH_DCYCLE = (uint32_t)(700/duty_unit);

static bool change_color(gptimer_handle_t, const gptimer_alarm_event_data_t*, void*);
void init_led();
void set_color(unsigned int);

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
    .duty = 0,
    .hpoint = 90
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

void init_led(){
    ESP_ERROR_CHECK(ledc_timer_config(&led_timer_config));
    ESP_ERROR_CHECK(ledc_channel_config(&led_channel_config));
    ESP_ERROR_CHECK(gptimer_new_timer(&color_timer_config, &color_timer_handler));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(color_timer_handler, &color_alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(color_timer_handler, &color_cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(color_timer_handler));
    ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
    color_duties[0] = 0;
}

static bool change_color(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, color_duties[color_weight]);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    if(color_weight == 0){
        color_weight = 74;
        gptimer_stop(timer);
        return true;
    }
    color_weight--;
    return true;
}

//0x000000 to 0xffffff (GRB)
void set_color(unsigned int new_color){
    int i;
    for(i = 0; i < 75; i++){
        if(i <= 50){
            color_duties[i] = 0;
        }else{
            color_duties[i] = (bool)((new_color >> (i-51)) & 1) ? HIGH_DCYCLE : LOW_DCYCLE;
        }
    }
    ESP_ERROR_CHECK(gptimer_start(color_timer_handler));
}

void app_main() {
    init_led();
    color = 1;
    set_color(color);
    while(true){
        vTaskDelay(5);
        color = color << 1;
        if(color > 0xFFFFFF) color = 1;
        set_color(color);
    };
}