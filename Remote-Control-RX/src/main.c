#include <driver/gpio.h>
#include <driver/rmt_rx.h>
#include <driver/rmt_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_err.h>
#include <stdio.h>
#include <stdbool.h>

#define NEC_LEADING_CODE_DURATION_0  9000
#define NEC_LEADING_CODE_DURATION_1  4500
#define NEC_PAYLOAD_ZERO_DURATION_0  560
#define NEC_PAYLOAD_ZERO_DURATION_1  560
#define NEC_PAYLOAD_ONE_DURATION_0   560
#define NEC_PAYLOAD_ONE_DURATION_1   1690
#define NEC_REPEAT_CODE_DURATION_0   9000
#define NEC_REPEAT_CODE_DURATION_1   2250

#define NEC_DECODE_MARGIN 200 

static inline bool nec_check_in_range(uint32_t signal_duration, uint32_t spec_duration)
{
    return (signal_duration < (spec_duration + NEC_DECODE_MARGIN)) &&
           (signal_duration > (spec_duration - NEC_DECODE_MARGIN));
}

static bool rmt_rx_callback(rmt_channel_handle_t channel, const rmt_rx_done_event_data_t *edata, void *usr_data){
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)usr_data;
    // send the received RMT symbols to the parser task
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    // return whether any task is woken up
    return high_task_wakeup == pdTRUE;
}

void app_main() {
    rmt_channel_handle_t rx_chan = NULL;
    rmt_rx_channel_config_t rx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,   // select source clock
        .resolution_hz = 1 * 1000 * 1000, // 1 MHz tick resolution, i.e., 1 tick = 1 µs
        .mem_block_symbols = 64,          // memory block size, 64 * 4 = 256 Bytes
        .gpio_num = GPIO_NUM_2,           // GPIO number
        .flags.invert_in = false,         // do not invert input signal
        .flags.with_dma = false,          // do not need DMA backend
    };
    ESP_ERROR_CHECK(rmt_new_rx_channel(&rx_chan_config, &rx_chan));
    ESP_ERROR_CHECK(rmt_enable(rx_chan));

    QueueHandle_t receive_queue = xQueueCreate(1, sizeof(rmt_rx_done_event_data_t));
    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = rmt_rx_callback,
    };

    ESP_ERROR_CHECK(rmt_rx_register_event_callbacks(rx_chan, &cbs, receive_queue));

    rmt_receive_config_t receive_config = {
        .signal_range_min_ns = 500,     // the shortest duration for NEC signal is 560 µs, 1250 ns < 560 µs, valid signal is not treated as noise
        .signal_range_max_ns = 12000000, // the longest duration for NEC signal is 9000 µs, 12000000 ns > 9000 µs, the receive does not stop early
    };
    
    rmt_carrier_config_t rx_carrier_cfg = {
        .duty_cycle = 0.33,                 // duty cycle 33%
        .frequency_hz = 25000,              // carrier
        .flags.polarity_active_low = false, // the carrier is modulated to high level
    };
    // demodulate carrier from RX channel
    ESP_ERROR_CHECK(rmt_apply_carrier(rx_chan, &rx_carrier_cfg));

    rmt_symbol_word_t raw_symbols[64]; // 64 symbols should be sufficient for a standard NEC frame
    rmt_rx_done_event_data_t rx_data;

    int i;
    bool leading_check, zero_check, one_check, repeat_check;




    
    
    while(true){
        ESP_ERROR_CHECK(rmt_receive(rx_chan, raw_symbols, sizeof(raw_symbols), &receive_config));
        xQueueReceive(receive_queue, &rx_data, portMAX_DELAY);
        
        uint32_t message = 0;

        for(i = 0; i < rx_data.num_symbols; i++){
            leading_check = i==0 && \
                nec_check_in_range(rx_data.received_symbols[i].duration0, NEC_LEADING_CODE_DURATION_0) && \
                nec_check_in_range(rx_data.received_symbols[i].duration1, NEC_LEADING_CODE_DURATION_1);
            zero_check = nec_check_in_range(rx_data.received_symbols[i].duration0, NEC_PAYLOAD_ZERO_DURATION_0) && \
                         nec_check_in_range(rx_data.received_symbols[i].duration1, NEC_PAYLOAD_ZERO_DURATION_1);
            one_check = nec_check_in_range(rx_data.received_symbols[i].duration0, NEC_PAYLOAD_ONE_DURATION_0) && \
                        nec_check_in_range(rx_data.received_symbols[i].duration1, NEC_PAYLOAD_ONE_DURATION_1);
            repeat_check = nec_check_in_range(rx_data.received_symbols[i].duration0, NEC_REPEAT_CODE_DURATION_0) && \
                           nec_check_in_range(rx_data.received_symbols[i].duration1, NEC_REPEAT_CODE_DURATION_1);
            if(leading_check)
                continue;
            if(one_check)
                    message = (message << 1) | 1;
            else if(zero_check)
                message = (message << 1) & 0xFFFFFFFE;
            else if(repeat_check)
                message = 0;
        }
        printf("%lx\n", message);

        vTaskDelay(pdMS_TO_TICKS(1));
    }
} 