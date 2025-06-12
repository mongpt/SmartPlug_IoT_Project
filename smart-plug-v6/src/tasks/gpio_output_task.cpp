//
// Created by ADMIN on 1/30/2025.
//

#include "gpio_output_task.h"

#include <cstdio>

void gpio_ouput_task(void *pvParameters) {
    auto *gpio = static_cast<gpio_out_s *>(pvParameters);

    uint8_t state = 0;
    uint8_t last_state = 0;
    EventBits_t state_bits;

    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_init(LED_GR);
    gpio_set_dir(LED_GR, GPIO_OUT);
    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_OUT);
    gpio_init(FF_SD);
    gpio_set_dir(FF_SD, GPIO_OUT);

    gpio_put(TRIGGER_PIN, false);
    gpio_put(LED_R, true);
    gpio_put(LED_GR, false);
    gpio_put(FF_SD, false);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_put(FF_SD, true);

    while (true) {

        // check if device state has changed
        state_bits = xEventGroupWaitBits(state_event_group, STATE_OUTPUT_BIT,
                                               pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (state_bits & STATE_OUTPUT_BIT) {
            xQueuePeek(gpio->dev_state_q, &state, pdMS_TO_TICKS(10));
        }
        if (xSemaphoreTake(gpio->output_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
            state = !state;
            // empty the queue before adding new data
            xQueueReset(gpio->dev_state_q);
            // send the new state to a queue
            xQueueSend(gpio->dev_state_q, &state, portMAX_DELAY);
            // notify display task
            xEventGroupSetBits(state_event_group, STATE_DISPLAY_BIT);
        }
        if (xSemaphoreTake(gpio->ocp_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
            state = 0;
            // empty the queue before adding new data
            xQueueReset(gpio->dev_state_q);
            // send the new state to a queue
            xQueueSend(gpio->dev_state_q, &state, portMAX_DELAY);
            // notify display task
            xEventGroupSetBits(state_event_group, STATE_DISPLAY_BIT);
        }

        // check if flipflop needs to be reset
        if (xSemaphoreTake(gpio->flipflop_reset_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
            state = 0;
            // empty the queue before adding new data
            xQueueReset(gpio->dev_state_q);
            // send the new state to a queue
            xQueueSend(gpio->dev_state_q, &state, portMAX_DELAY);
            gpio_put(FF_SD, false);
            vTaskDelay(pdMS_TO_TICKS(1000));
            gpio_put(FF_SD, true);

            // notify display task
            xEventGroupSetBits(state_event_group, STATE_DISPLAY_BIT | STATE_BUZZER_BIT);
        }

        if (last_state != state) {
            last_state = state;
            // change device's state
            gpio_put(TRIGGER_PIN, state);
            gpio_put(LED_R, !state);
            gpio_put(LED_GR, state);
            //notify internet task to send state to cloud
            xQueueSend(gpio->state_to_internet_q, &state, pdMS_TO_TICKS(100));
            xSemaphoreGive(gpio->state_to_cloud_sem);
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
