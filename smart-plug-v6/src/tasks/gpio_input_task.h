//
// Created by ADMIN on 1/30/2025.
//

#ifndef GPIO_INPUT_TASK_H
#define GPIO_INPUT_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "queue.h"
#include "stdio.h"
#include "common.h"

#define AP 13   // turn on wifi Access Point
#define QR 12   // change QR code
#define SW 11   // on/off socket
#define OCP_IRQ 19  // overcurrent protection
#define ALARM_INT_PIN 10  // Connect MCP79410 MFP pin to GPIO 10

#define DEBOUNCE_MS 200

void gpio_callback(uint gpio, uint32_t events);
void setupPin(uint8_t pin);
void gpio_input_task(void *pvParameters);

#endif //GPIO_INPUT_TASK_H
