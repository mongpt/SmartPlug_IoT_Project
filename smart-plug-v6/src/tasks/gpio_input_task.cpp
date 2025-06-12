#include "gpio_input_task.h"

QueueHandle_t gpio_event_q = xQueueCreate(5, sizeof(EventType));

void gpio_callback(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    EventType event = NONE;

    switch (gpio) {
        case AP:
            event = AP_BTN;
            break;
        case QR:
            event = QR_BTN;
            break;
        case SW:
            event = SW_BTN;
            break;
        case OCP_IRQ:
            event = OCP_TRIGGER;
            break;
        case ALARM_INT_PIN:
            event = ALARM_INT;
            break;
        default:
            return;
    }
    xQueueSendFromISR(gpio_event_q, &event, &xHigherPriorityTaskWoken);
//    printf("GPIO %d triggered\n", gpio);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void setupPin(uint8_t pin) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    gpio_set_irq_enabled_with_callback(pin, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
}

void gpio_input_task(void *pvParameters) {
    auto *gpio_data = static_cast<gpio_in_s*>(pvParameters);  // Store the passed struct in global pointer

    EventType event = NONE;
    uint64_t start_time;
    uint64_t last_time = time_us_64();
    uint8_t count;
    // Setup GPIO pins for input with interrupts
    setupPin(AP);
    setupPin(QR);
    setupPin(SW);
    setupPin(OCP_IRQ);
    setupPin(ALARM_INT_PIN);

    while (true) {

        if (xQueueReceive(gpio_event_q, &event, portMAX_DELAY) == pdTRUE) {
            start_time = time_us_64();
            uint8_t beep = 0;
            if (start_time - last_time < DEBOUNCE_MS * 1000) {
                continue;
            } else {
                last_time = start_time;
                switch (event) {
                    case SW_BTN:
                        beep = 1;
                        xSemaphoreGive(gpio_data->output_sem);
                        xQueueSend(gpio_data->gpio_in_q, &beep, pdMS_TO_TICKS(0));
                        break;
                    case OCP_TRIGGER:
                        beep = 3;
                        xSemaphoreGive(gpio_data->ocp_sem);
                        xQueueSend(gpio_data->gpio_in_q, &beep, pdMS_TO_TICKS(0));
                        break;
                    case QR_BTN:
                        beep = 1;
                        xSemaphoreGive(gpio_data->qr_sem);
                        xQueueSend(gpio_data->gpio_in_q, &beep, pdMS_TO_TICKS(0));
                        break;
                    case AP_BTN:
                        count = 0;
                        while (!gpio_get(AP) && ++count < 150){
                            vTaskDelay(pdMS_TO_TICKS(10));
                        }
                        if (count >= 150){ //long press to reset network
                            beep = 2;
                            xSemaphoreGive(gpio_data->wifi_reset_sem);
                            xQueueSend(gpio_data->gpio_in_q, &beep, pdMS_TO_TICKS(0));
                        } else {    // notify gpio output task to reset flipflop
                            beep = 1;
                            xSemaphoreGive(gpio_data->flipflop_reset_sem);
                            xQueueSend(gpio_data->gpio_in_q, &beep, pdMS_TO_TICKS(0));
                        }
                        break;
                        case ALARM_INT:
                            xSemaphoreGive(gpio_data->alarm_sem);
                            break;
                    default:
                        break;
                }
            }
        }

//        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
