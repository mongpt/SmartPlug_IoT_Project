//
// Created by ADMIN on 2/15/2025.
//

#include "buzzer_task.h"

void buzzer_task(void *pvParameters) {
    auto *data = static_cast<buzzer_s *>(pvParameters);

    uint8_t beep;
    EventBits_t state_bits;

    uint8_t state;

    // PWM setup for the buzzer (4 kHz PWM signal)
    uint wrap = 249;  // 4 kHz PWM
    uint duty = 50;   // 50% duty cycle
    uint div = 125;   // Clock divider (1 MHz)
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);
    uint chan = pwm_gpio_to_channel(BUZZER);
    pwm_set_enabled(slice_num, false);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv_int(&cfg, div); // 1 MHz clock
    pwm_config_set_wrap(&cfg, wrap);
    pwm_init(slice_num, &cfg, false);
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    pwm_set_enabled(slice_num, true);

    while (true) {
        // check if device state has changed
        state_bits = xEventGroupWaitBits(state_event_group, STATE_BUZZER_BIT,
                                               pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (state_bits & STATE_BUZZER_BIT) {
            if (xQueuePeek(data->dev_state_q, &state, pdMS_TO_TICKS(0)) == pdTRUE) {
                uint8_t n = 0;
                // change device's state
                if (state == 1) {
                    n = 2;
                    for (uint8_t i = 0; i < n; i++) {
                        pwm_set_chan_level(slice_num, chan, (wrap+1) * duty / 100); // 50% duty cycle
                        vTaskDelay(pdMS_TO_TICKS(100));
                        pwm_set_chan_level(slice_num, chan, 0); // 0% duty cycle
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                } else if (state == 0) {
                    n = 3;
                    for (uint8_t i = 0; i < n; i++) {
                        pwm_set_chan_level(slice_num, chan, (wrap+1) * duty / 100); // 50% duty cycle
                        vTaskDelay(pdMS_TO_TICKS(500));
                        pwm_set_chan_level(slice_num, chan, 0); // 0% duty cycle
                        vTaskDelay(pdMS_TO_TICKS(100));
                    }
                }
            }
        }

        // loop forever if ocp is triggered
        if (state == 2) {
            pwm_set_chan_level(slice_num, chan, (wrap+1) * duty / 100); // 50% duty cycle
            vTaskDelay(pdMS_TO_TICKS(250));
            pwm_set_chan_level(slice_num, chan, 0); // 0% duty cycle
            vTaskDelay(pdMS_TO_TICKS(250));
        }

        // check if gpio event is received
        if (xQueueReceive(data->gpio_in_q, &beep, pdMS_TO_TICKS(0)) == pdTRUE) {
            for (uint8_t i = 0; i < beep; i++) {
                pwm_set_chan_level(slice_num, chan, (wrap+1) * duty / 100); // 50% duty cycle
                vTaskDelay(pdMS_TO_TICKS(100));
                pwm_set_chan_level(slice_num, chan, 0); // 0% duty cycle
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}