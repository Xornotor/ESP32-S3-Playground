#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/freertos.h"
#include "freertos/task.h"

#define HIGH 1
#define LOW 0

#define RS GPIO_NUM_3
#define EN GPIO_NUM_46
#define D4 GPIO_NUM_9
#define D5 GPIO_NUM_10
#define D6 GPIO_NUM_11
#define D7 GPIO_NUM_12

void cmd_16x2(bool D7_value, bool D6_value, bool D5_value, bool D4_value){
    gpio_set_level(RS, LOW);
    gpio_set_level(D7, D7_value);
    gpio_set_level(D6, D6_value);
    gpio_set_level(D5, D5_value);
    gpio_set_level(D4, D4_value);
    gpio_set_level(EN, HIGH);
    vTaskDelay(10);
    gpio_set_level(EN, LOW);
}

void write_16x2(bool D7_value, bool D6_value, bool D5_value, bool D4_value){
    gpio_set_level(RS, HIGH);
    gpio_set_level(D7, D7_value);
    gpio_set_level(D6, D6_value);
    gpio_set_level(D5, D5_value);
    gpio_set_level(D4, D4_value);
    gpio_set_level(EN, HIGH);
    vTaskDelay(10);
    gpio_set_level(EN, LOW);
}

void init_16x2(void){
    //Pin config
    gpio_set_direction(RS, GPIO_MODE_OUTPUT);
    gpio_set_direction(EN, GPIO_MODE_OUTPUT);
    gpio_set_direction(D4, GPIO_MODE_OUTPUT);
    gpio_set_direction(D5, GPIO_MODE_OUTPUT);
    gpio_set_direction(D6, GPIO_MODE_OUTPUT);
    gpio_set_direction(D7, GPIO_MODE_OUTPUT);

    //Reset RS and EN pin values
    gpio_set_level(RS, LOW);
    gpio_set_level(EN, LOW);

    //Function set - 4-Bits, 2 Rows, 5x7 char size
    cmd_16x2(LOW, LOW, HIGH, LOW);
    cmd_16x2(HIGH, LOW, LOW, LOW);

    //Screen clear
    cmd_16x2(LOW, LOW, LOW, LOW);
    cmd_16x2(LOW, LOW, LOW, HIGH);

    //Display On, Cursor On, Blink On
    cmd_16x2(LOW, LOW, LOW, LOW);
    cmd_16x2(HIGH, HIGH, HIGH, HIGH);

    //TEXTO

    //A
    write_16x2(LOW, HIGH, LOW, LOW);
    write_16x2(LOW, LOW, LOW, HIGH);
    //n
    write_16x2(LOW, HIGH, HIGH, LOW);
    write_16x2(HIGH, HIGH, HIGH, LOW);
    //d
    write_16x2(LOW, HIGH, HIGH, LOW);
    write_16x2(LOW, HIGH, LOW, LOW);

    //underline
    write_16x2(LOW, HIGH, LOW, HIGH);
    write_16x2(HIGH, HIGH, HIGH, HIGH);

    //X
    write_16x2(LOW, HIGH, LOW, HIGH);
    write_16x2(HIGH, LOW, LOW, LOW);
    //o
    write_16x2(LOW, HIGH, HIGH, LOW);
    write_16x2(HIGH, HIGH, HIGH, HIGH);
    //r
    write_16x2(LOW, HIGH, HIGH, HIGH);
    write_16x2(LOW, LOW, HIGH, LOW);

    //underline
    write_16x2(LOW, HIGH, LOW, HIGH);
    write_16x2(HIGH, HIGH, HIGH, HIGH);

    //n
    write_16x2(LOW, HIGH, HIGH, LOW);
    write_16x2(HIGH, HIGH, HIGH, LOW);
    //o
    write_16x2(LOW, HIGH, HIGH, LOW);
    write_16x2(HIGH, HIGH, HIGH, HIGH);
    //t
    write_16x2(LOW, HIGH, HIGH, HIGH);
    write_16x2(LOW, HIGH, LOW, LOW);

    //underline
    write_16x2(LOW, HIGH, LOW, HIGH);
    write_16x2(HIGH, HIGH, HIGH, HIGH);

    //o
    write_16x2(LOW, HIGH, HIGH, LOW);
    write_16x2(HIGH, HIGH, HIGH, HIGH);
    //r
    write_16x2(LOW, HIGH, HIGH, HIGH);
    write_16x2(LOW, LOW, HIGH, LOW);
    
}

void app_main(void)
{
    init_16x2();

    while(true){
        vTaskDelay(10);
    }
}
