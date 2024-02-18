#include <stdio.h>
#include <rom/ets_sys.h>
#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

int color_weight = 24;
int color = 0xffffff;

static bool change_color(gptimer_handle_t, const gptimer_alarm_event_data_t*, void*);
void init_color_timer();
void set_color(uint32_t);

const ledc_timer_config_t led_timer_config = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_8_BIT,
    .timer_num = LEDC_TIMER_0,
    .freq_hz = 60000,
    .clk_cfg = LEDC_AUTO_CLK
};

const ledc_channel_config_t led_channel_config = {
    .gpio_num = 48,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = LEDC_CHANNEL_0,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = LEDC_TIMER_0,
    .duty = 8,
    .hpoint = 0
};

gptimer_handle_t color_timer_handler = NULL;

const gptimer_config_t color_timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 60000
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
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(color_timer_handler, &color_cbs, &color_weight));
    ESP_ERROR_CHECK(gptimer_enable(color_timer_handler));
}

static bool change_color(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    printf("Callback\n");
    if(color_weight <= 0){
        gptimer_stop(timer);
        ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
        return true;
    }
    int color_bit = (bool)((color >> (24 - (int)(user_ctx))) & 1) ? 11 : 4;
    printf("%d", color_bit);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, color_bit);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    color_weight--;
    return true;
}

//0x000000 to 0xffffff (GRB)
void set_color(uint32_t new_color){
    color = new_color;
    color_weight = 24;

    ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
    ets_delay_us(100);
    printf("Antes do gptimer_start\n");
    ESP_ERROR_CHECK(gptimer_start(color_timer_handler));
    printf("Depois do gptimer_start\n");
    ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
    printf("Depois do timer_resume\n");

    //ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 11);
    //ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    //ets_delay_us(160);
    //ledc_timer_pause(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);

}

void app_main() {
    ESP_ERROR_CHECK(ledc_timer_config(&led_timer_config));
    ESP_ERROR_CHECK(ledc_channel_config(&led_channel_config));

    init_color_timer();
    set_color(0x00FF00);

    while(true){
        vTaskDelay(100);
    };

}