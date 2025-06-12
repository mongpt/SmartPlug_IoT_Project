//
// Created by ADMIN on 1/19/2025.
//

#include "display_task.h"

static void format_datetime(uint64_t timestamp, char *buffer, size_t bufferSize) {
    uint8_t month, day, hour, minute;

    // Extract values
    minute = timestamp % 100;
    hour = (timestamp / 100) % 100;
    day = (timestamp / 10000) % 100;
    month = (timestamp / 1000000) % 100;

    // Format as MM/DD/YYYY HH:MM
    snprintf(buffer, bufferSize, "%hhu/%hhu %02hhu:%02hhu", month, day, hour, minute);
}

void display_task(void *pvParameters){
    auto *data = static_cast<display_data_s *>(pvParameters);

    time_only_s clock = {.hour = 0, .minute = 0};
    uint8_t bat_value = 0;
    uint8_t wifi_state = 0;
    uint8_t device_state = 0;
    float setpoint = 0.0;
    float kWh = 0.0;
    uint8_t qr_type = 2;
    float load_voltage = 12.0;
    float load_current = 0.0;
    float load_power = 0.0;

    EventBits_t time_bits;
    EventBits_t setpoint_bits;
    EventBits_t state_bits;
    EventBits_t adc_bits;

// lcd configuration
    ST7789::Config lcd_config = {
            .spi      = PICO_DEFAULT_SPI_INSTANCE,
            .gpio_din = 3,
            .gpio_clk = 2,
            .gpio_cs  = 5,
            .gpio_dc  = 4,
            .gpio_rst = 6,
            .gpio_bl  = 7,
    };
// Create an ST7789 instance
    ST7789 *lcd = new ST7789(lcd_config);
    // Initialize the LCD display
    lcd->init();

    lcd->setRotation(2);

    SCREEN display(lcd);

    display.showFrame();

    display.showClock(clock.hour, clock.minute);
    display.showBAT(bat_value);
    display.showWIFI(wifi_state);
    display.showState(device_state);
    display.showTimer("--/-- --:--", true);
    display.showVoltage(load_voltage);
    display.showCurrent(load_current);
    display.showPower(load_power);
    display.showSetPw(setpoint);
    display.showQR(qr_type);
    display.showConsumption(kWh);

    while (true) {

        // check if wifi status has changed
        if (xQueuePeek(data->wifi_status_q, &wifi_state, pdMS_TO_TICKS(0)) == pdTRUE) {
            display.showWIFI(wifi_state);
        }

        // check if time has changed
        if (xQueueReceive(data->clock_data_q, &clock, pdMS_TO_TICKS(0)) == pdTRUE) {
            display.showClock(clock.hour, clock.minute);
        }

        // check if load current has changed
        if (xQueueReceive(data->adc_current_q, &load_current, pdMS_TO_TICKS(0)) == pdTRUE) {
            //printf("Current value: %.1f\n", current);
            display.showCurrent(load_current);
        }

        // check if load power has changed
        if (xQueueReceive(data->adc_power_q, &load_power, pdMS_TO_TICKS(0)) == pdTRUE) {
            //printf("Power value: %.1f\n", current);
            display.showPower(load_power);
        }

        // check if rtc bat level has changed
        if (xQueueReceive(data->adc_bat_q, &bat_value, pdMS_TO_TICKS(0)) == pdTRUE) {
//            printf("received bat value: %.1f\n", bat_value);
            // update display
            display.showBAT(bat_value);
        }

        // check if load voltage has changed
        if (xQueueReceive(data->adc_voltage_q, &load_voltage, pdMS_TO_TICKS(0)) == pdTRUE) {
            //printf("Voltage value: %.1f\n", load_voltage);
            display.showVoltage(load_voltage);
        }

        // check if new setpoint is available
        setpoint_bits = xEventGroupWaitBits(setpoint_event_group, SETPOINT_DISPLAY_BIT,
                                               pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (setpoint_bits & SETPOINT_DISPLAY_BIT) {
            if (xQueuePeek(data->setpoint_q, &setpoint, pdMS_TO_TICKS(0)) == pdTRUE) {
                // update display
                display.showSetPw(setpoint);
            }
        }
        // check if device state has changed
        state_bits = xEventGroupWaitBits(state_event_group, STATE_DISPLAY_BIT,
                                                     pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (state_bits & STATE_DISPLAY_BIT) {
            if (xQueuePeek(data->dev_state_q, &device_state, pdMS_TO_TICKS(0)) == pdTRUE) {
                // update display
                display.showState(device_state);
            }
        }
        // check if QR button has been pressed
        if (xSemaphoreTake(data->qr_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
            qr_type = (qr_type + 1) % 3;
            display.showQR(qr_type);
        }
        // check if new power consumption data is available
        if (xQueueReceive(data->pw_consumed_q, &kWh, pdMS_TO_TICKS(0)) == pdTRUE) {
            display.showConsumption(kWh);
        }
        // Check if new timers need to be displayed
        time_bits = xEventGroupWaitBits(time_event_group, TIME_DISPLAY_BIT,
                                   pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (time_bits & TIME_DISPLAY_BIT) {
            uint64_t timer;
            char buffer[17] = {0};
            uint8_t state = 1;
            // get data from queue
            if (xQueuePeek(data->time_q, &timer, pdMS_TO_TICKS(0)) == pdTRUE) {
                if (timer == 99123123592) {
                    memcpy(buffer, "--/-- --:--", 12);
                } else {
                    uint64_t timer1 = timer / 10;
                    state = timer % 10;
                    format_datetime(timer1, buffer, sizeof(buffer));
//                    printf("debug: buffer: %s\n", buffer);
                }
                // display timers
                display.showTimer(buffer, state);
            }
        }

        // check if OTA is being triggered
        if (xSemaphoreTake(data->ota_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
            display.showOTA();
        }

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}