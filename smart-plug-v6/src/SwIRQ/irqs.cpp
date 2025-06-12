#include "irqs.h"

//gpio_in_s *irqs::gpio_data = nullptr;

irqs::irqs(uint8_t PinA, gpio_in_s *gpio_in_data) { // Constructor
    gpio_data = gpio_in_data;  // Store queue handles
    setupPin(PinA);
}

void irqs::setupPin(uint8_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &callback);
}

void irqs::callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventType event;
    static uint32_t lastButtonPressTime = 0;
    uint32_t currentTime = to_ms_since_boot(get_absolute_time());

    if (gpio == OCP_IRQ) {
        event = OCP_TRIGGER;
    } else if (gpio == SELECT && (currentTime - lastButtonPressTime) > BUTTON_DEBOUNCE_DELAY_MS) {
        event = SELECT_BTN;
        lastButtonPressTime = currentTime;
    } else if (gpio == UP && (currentTime - lastButtonPressTime) > BUTTON_DEBOUNCE_DELAY_MS) {
        event = UP_BTN;
        lastButtonPressTime = currentTime;
    } else if (gpio == DOWN && (currentTime - lastButtonPressTime) > BUTTON_DEBOUNCE_DELAY_MS) {
        event = DOWN_BTN;
        lastButtonPressTime = currentTime;
    } else {
        return; // Ignore if debounce condition is not met
    }

    // Send event to both display and output tasks
    xQueueSendFromISR(gpio_data->to_display_q, &event, &xHigherPriorityTaskWoken);
    xQueueSendFromISR(gpio_data->to_output_q, &event, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
