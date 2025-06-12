//
// Created by iamna on 05/10/2024.
//

#ifndef RP2040_FREERTOS_IRQ_IRQS_H
#define RP2040_FREERTOS_IRQ_IRQS_H
#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "hardware/gpio.h"

#define BUTTON_DEBOUNCE_DELAY_MS 30 // replace with your actual delay time in milliseconds

class irqs {
public:
    typedef enum {
        SELECT_BTN,
        UP_BTN,
        DOWN_BTN,
        OCP_TRIGGER
    } EventType;
    EventType event;
    //struct gpio_in_s;
    static gpio_in_s gpio_data;
    explicit irqs(uint8_t PinA, gpio_in_s *gpio_data);
    void setupPin(uint8_t pin);
    static void callback(uint gpio, uint32_t events);
};


#endif //RP2040_FREERTOS_IRQ_IRQS_H
