#include <stdio.h>
#include <math.h>
#include <rom/ets_sys.h>
#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define FREQ_HZ         40000
#define DUTY_RESOLUTION 7
const double duty_unit = (1e9/(FREQ_HZ * 128));
const uint32_t LOW_DCYCLE = (uint32_t)(300/duty_unit);
const uint32_t HIGH_DCYCLE = (uint32_t)(700/duty_unit);

uint8_t color_weight = 74;
unsigned int color = 0x000001;
uint32_t color_duties[75];
uint8_t color_rainbow_state = 0;

static bool change_color(gptimer_handle_t, const gptimer_alarm_event_data_t*, void*);
void init_led();
void set_color(unsigned int);
void color_shift();
void color_rainbow();

const ledc_timer_config_t led_timer_config = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = DUTY_RESOLUTION,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = FREQ_HZ,
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
    .resolution_hz = FREQ_HZ
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

void color_shift(){
    color = color << 1;
    if(color > 0xFFFFFF) color = 1;
    set_color(color);
}

void color_rainbow(){
    switch(color_rainbow_state){
        case 0:
            if(color == 1) color = 0;
            else color += 0x100;
            if(color == 0xFF00){
                color_rainbow_state = 1;
            }
            break;
        case 1:
            color += 0x10000;
            if(color == 0xFFFF00){
                color_rainbow_state = 2;
            }
            break;
        case 2:
            color -= 0x100;
            if(color == 0xFF0000){
                color_rainbow_state = 3;
            }
            break;
        case 3:
            color += 1;
            if(color == 0xFF00FF){
                color_rainbow_state = 4;
            }
            break;
        case 4:
            color -= 0x10000;
            if(color == 0xFF){
                color_rainbow_state = 5;
            }
            break;
        case 5:
            color += 0x100;
            if(color == 0xFFFF){
                color_rainbow_state = 6;
            }
            break;
        case 6:
            color -= 1;
            if(color == 0xFF00){
                color_rainbow_state = 7;
            }
            break;
        case 7:
            color -= 0x100;
            if(color == 0){
                color_rainbow_state = 0;
            }
            break;
    }
    set_color(color);
}

void app_main() {
    init_led();
    while(true){
        color_rainbow();
        vTaskDelay(1);
    };
}