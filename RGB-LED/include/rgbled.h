#include <stdio.h>
#include "driver/ledc.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Constantes de Configuração
#define FREQ_HZ         40000
#define DUTY_RESOLUTION LEDC_TIMER_7_BIT
#define HPOINT          90
const double duty_unit = (1e9/(FREQ_HZ * 128));
const uint32_t LOW_DCYCLE = (uint32_t)(300/duty_unit);
const uint32_t HIGH_DCYCLE = (uint32_t)(700/duty_unit);

// Variáveis globais para funções específicas
unsigned int color = 0x000001;
uint8_t color_rainbow_state = 0;

//Protótipos de funções
static bool change_color(gptimer_handle_t, const gptimer_alarm_event_data_t*, void*);
void init_led();
void set_color(unsigned int);
void color_shift();
void color_rainbow();

// Handler de fila para gerenciamento de cor
QueueHandle_t xQueueColor = NULL;

// Configurações da controladora de LED
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
    .hpoint = HPOINT
};

// Configurações de timer para mudança de cores
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

// Função de inicialização
void init_led(){
    ESP_ERROR_CHECK(ledc_timer_config(&led_timer_config));
    ESP_ERROR_CHECK(ledc_channel_config(&led_channel_config));
    ESP_ERROR_CHECK(gptimer_new_timer(&color_timer_config, &color_timer_handler));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(color_timer_handler, &color_alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(color_timer_handler, &color_cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(color_timer_handler));
    xQueueColor = xQueueCreate(225, sizeof(uint16_t));
    ledc_timer_resume(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0);
}

// Função de geração de pulsos para mudança de cor (Não chamar diretamente)
static bool change_color(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
    uint16_t d_cycle = 0;
    if(xQueueReceive(xQueueColor, &d_cycle, 0) == errQUEUE_EMPTY){
        gptimer_stop(timer);
    }
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, d_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    return true;
}

// Função de seleção de cor - 0x000000 to 0xffffff (GRB)
void set_color(unsigned int new_color){
    uint8_t i;
    uint16_t color_duty = 0;
    for(i = 0; i < 75; i++){
        if(i < 24){
            color_duty = (bool)((new_color >> (23-i)) & 1) ? HIGH_DCYCLE : LOW_DCYCLE;
        }
        if(xQueueSendToBack(xQueueColor, &color_duty, 0) == errQUEUE_FULL){
            printf("WARNING: QUEUE FULL");
        }
    }
    ESP_ERROR_CHECK(gptimer_start(color_timer_handler));
}

// Função de color shift (feita para rodar em loop)
void color_shift(){
    color = color << 1;
    if(color > 0xFFFFFF) color = 1;
    set_color(color);
}

// Função de color rainbow (feita para rodar em loop)
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