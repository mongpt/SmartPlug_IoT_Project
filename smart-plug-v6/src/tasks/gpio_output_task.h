//
// Created by ADMIN on 1/30/2025.
//

#ifndef GPIO_OUTPUT_TASK_H
#define GPIO_OUTPUT_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include <queue.h>
#include "semphr.h"
#include "common.h"

#define LED_R 14
#define LED_GR 15
#define TRIGGER_PIN 16  // control relay
#define FF_SD 19    // set to LOW at boot to set Q output of flipflop to HIGH

void gpio_ouput_task(void *pvParameters);

#endif //GPIO_OUTPUT_TASK_H
