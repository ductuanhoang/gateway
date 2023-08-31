#include "button.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define GPIO_INPUT_IO_0 0
#define GPIO_INPUT_PIN_SEL ((1ULL << GPIO_INPUT_IO_0))
#define BUTTON_TAG "BUTTON"

#define USER_BUTTON_HOLD (500)
#define USER_BUTTON_DOWN 0
#define USER_BUTTON_UP 1

#define GPIO_CONTROL_RELAY 32
#define GPIO_CONTROL_RELAY_SEL ((1ULL << GPIO_CONTROL_RELAY))

static void user_button_task(void *arg);

static button_callback_t callback;
static uint8_t button_state = 0;
static uint32_t button_state_current_time = 0;

void user_button_init(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = GPIO_CONTROL_RELAY_SEL;
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);

    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // bit mask of the pins, use GPIO4/5 here
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    xTaskCreate(user_button_task, "user_button_task", 2048, NULL, 10, NULL);
}

/**
 * @brief
 *
 * @param btn_callback
 */
void user_button_callback_init(button_callback_t btn_callback)
{
    if (btn_callback == NULL)
        return;
    callback = btn_callback;
}

/**
 * @brief
 *
 * @param state
 */
void user_control_relay(uint8_t state)
{
    gpio_set_level(GPIO_CONTROL_RELAY,state);
}

static void user_button_task(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if ((gpio_get_level(GPIO_INPUT_IO_0) == USER_BUTTON_UP))
            button_state = 0;
        else if ((gpio_get_level(GPIO_INPUT_IO_0) == USER_BUTTON_DOWN) && (button_state == 0))
        {
            button_state = 1;
            button_state_current_time = xTaskGetTickCount();
            ESP_LOGI(BUTTON_TAG, "button_state_current_time = %d", button_state_current_time);
        }

        if ((button_state == 1) && (((xTaskGetTickCount()) - button_state_current_time) > USER_BUTTON_HOLD) && (gpio_get_level(GPIO_INPUT_IO_0) == USER_BUTTON_DOWN))
        {
            callback(0, E_USER_BUTTON_HOLD);
            button_state_current_time = xTaskGetTickCount();
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}