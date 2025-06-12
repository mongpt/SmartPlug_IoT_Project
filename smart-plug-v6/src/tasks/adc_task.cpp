//
// Created by ADMIN on 1/20/2025.
//

#include "adc_task.h"

uint8_t map_vol_to_percent(float adc_vol, float min_vol, float max_vol) {
    // Clamp the input voltage to the specified range
    if (adc_vol <= min_vol) return 0;  // Below range maps to 0%
    if (adc_vol >= max_vol) return 100; // Above range maps to 100%

    return ((adc_vol - min_vol) / (max_vol - min_vol)) * 100;
}

void adc_task(void *pvParameters) {
    auto *data = static_cast<adc_s *> (pvParameters);
    typedef struct {
        float total_energy_wh;  // Accumulated energy in wh
        TickType_t last_timestamp; // Last measurement timestamp
    } EnergyMeter;
    EnergyMeter meter = {0.0f, 0};

    float const no_load_vol = 1.237;
    float const sensitivity = 0.180; // 185mV/A for ACS712ELCTR-05B-T
    float i_sense_vol = 0.0;
    float vBat = 0.0;
    float setpoint = 54.0; // Default setpoint to max power
    float load_power = 0.0;
    float load_voltage = 12.0;
    float vol_offset = 15.57f;
    uint8_t adc0_offset = 10;
    uint8_t adc2_offset = 23;
    uint16_t raw_adc_value = 0;
    uint8_t wifi_state = 0;
    uint8_t dev_state = 0;  // combine with vout to determine if hw ocp is triggered

    EventBits_t setpoint_bits;
    EventBits_t time_bits;

    adc_init();
    adc_gpio_init(I_SENSE);
    adc_gpio_init(VBAT);
    adc_gpio_init(LOAD_VOL);

    TickType_t last_rtc_bat_read_time = 0; // Initialize last read time

    while (true) {
        // Check if new setpoint is received
        setpoint_bits = xEventGroupWaitBits(setpoint_event_group, SETPOINT_ADC_BIT,
                                            pdFALSE, pdFALSE, pdMS_TO_TICKS(0));
        if (setpoint_bits & SETPOINT_ADC_BIT) {
            if (xQueuePeek(data->setpoint_q, &setpoint, pdMS_TO_TICKS(0)) == pdTRUE) {
                xEventGroupClearBits(setpoint_event_group, SETPOINT_ADC_BIT);
                printf("Setpoint for adc: %.1f\n", setpoint);
            }
        }

        // Read load current
        adc_select_input(ADC_LOAD_CURRENT_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(10));
        i_sense_vol = 0;
        for (uint8_t i = 0; i < NUM_SAMPLES; i++) {
            raw_adc_value = adc_read();
            raw_adc_value = (raw_adc_value >= adc0_offset) ? (raw_adc_value - adc0_offset) : 0;
            i_sense_vol += ((float)raw_adc_value * ADC_VREF) / ADC_RESOLUTION;
            vTaskDelay(pdMS_TO_TICKS(1));
        }
//        printf("i_sense_vol: %.2f\n", i_sense_vol / NUM_SAMPLES);
        float diff_vol = i_sense_vol / NUM_SAMPLES - no_load_vol;
        float load_current = (diff_vol * 2) / sensitivity;
        load_current = (load_current < 0.0) ? 0.0 : load_current;
        if (load_current > MAX_CURRENT) {
            xSemaphoreGive(data->ocp_sem);
        }

        // Read RTC battery at boot, then every 24 hours
        TickType_t current_time = xTaskGetTickCount();
        if ((last_rtc_bat_read_time == 0) ||
            ((current_time - last_rtc_bat_read_time) >= pdMS_TO_TICKS(86400000))) {

            last_rtc_bat_read_time = current_time;

            adc_select_input(ADC_RTC_BAT_CHANNEL);
            vTaskDelay(pdMS_TO_TICKS(10));
            raw_adc_value = adc_read();
            vBat = (((float)raw_adc_value * ADC_VREF) / ADC_RESOLUTION) * 2;
//            printf("RTC Battery Voltage: %.2fV\n", vBat);
            uint8_t bat_percent = map_vol_to_percent(vBat, MIN_VBAT, MAX_VBAT);

            xQueueSend(data->adc_bat_q, &bat_percent, pdMS_TO_TICKS(0));
//            printf("RTC Battery Voltage: %.2fV, Battery Percentage: %d%%\n", vBat, bat_percent);
        }

        // Read load voltage and calculate power
        adc_select_input(ADC_LOAD_VOLTAGE_CHANNEL);
        vTaskDelay(pdMS_TO_TICKS(10));
        uint16_t adc_raw = adc_read();
        raw_adc_value = (adc_raw > adc2_offset) ? (adc_raw - adc2_offset) : 0;
        load_voltage = (((float)raw_adc_value * ADC_VREF) / ADC_RESOLUTION) * vol_offset;
        if (load_voltage == 0.0) {
            load_power = 0.0;
            // check dev state to determine if hw ocp is triggered
            xQueuePeek(data->dev_state_q, &dev_state, pdMS_TO_TICKS(0));
            if (dev_state == 1) {   // device is on but zero load voltage -> ocp
                // change dev state to 2
                dev_state = 2;  // overloaded state
                xQueueReset(data->dev_state_q);
                xQueueSend(data->dev_state_q, &dev_state, pdMS_TO_TICKS(0));
                // notify display and buzzer task
                xEventGroupSetBits(state_event_group, STATE_DISPLAY_BIT | STATE_BUZZER_BIT);
            }
        } else if (load_voltage > 0.0) {
            load_power = load_current * load_voltage;
        }
        // accumulate total wh
        if (meter.last_timestamp == 0) {
            meter.last_timestamp = xTaskGetTickCount();
        } else {
            TickType_t current_timestamp = xTaskGetTickCount();
            // Calculate time elapsed since last measurement (in seconds)
            float time_elapsed_s = (float )(current_timestamp - meter.last_timestamp) / 1000.0f; // Convert to second
            // Calculate energy in watt-seconds
            float energy_ws = load_power * time_elapsed_s;
            // Convert to watt-hours
            // 1 wh = 3,600 watt-seconds
            float energy_wh = energy_ws / 3600.0f;
            // Accumulate total energy
            meter.total_energy_wh += energy_wh;
//            printf("Total Energy Consumed: %.1f Wh\n", meter.total_energy_wh);
            // Update timestamp
            meter.last_timestamp = current_timestamp;
        }

        // check if consumed power is greater than setpoint
        if (load_power > setpoint) {    // turn off the device
            xSemaphoreGive(data->ocp_sem);
        } else {    // send data to display task
            xQueueSend(data->adc_current_q, &load_current, pdMS_TO_TICKS(0));
            xQueueSend(data->adc_power_q, &load_power, pdMS_TO_TICKS(0));
        }
        xQueueSend(data->adc_voltage_q, &load_voltage, pdMS_TO_TICKS(0));
//        printf("Load Current: %.2fA,Load Voltage: %.3fV ,Load Power: %.2fW\n", load_current,load_voltage,load_power);

        // check if it is time to send wh to cloud
        time_bits = xEventGroupWaitBits(time_event_group, TIME_TO_SEND_WH_ADC_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (time_bits & TIME_TO_SEND_WH_ADC_BIT) {
            // read time data in time_q that RTC sent
            uint64_t received_time = 0;
            xQueueReceive(data->time_q, &received_time, portMAX_DELAY);
            printf("Received time to send wh: %llu\n", received_time);
            // combine received time with total energy
            char wh_data[20] = {0};
            sprintf(wh_data, "%llu:%.1f", received_time, meter.total_energy_wh);
            printf("Data to send: %s\n", wh_data);

            //check wifi state before sending data
            xQueuePeek(data->wifi_status_q, &wifi_state, pdMS_TO_TICKS(0));
            if (wifi_state == 0) { //wifi disconnected -> store total wh to eeprom
                xQueueSend(data->wh_store_q, &wh_data, pdMS_TO_TICKS(100));
            } else {    //wifi connected -> send total wh to cloud
                xQueueSend(data->wh_send_q, &wh_data, pdMS_TO_TICKS(100));
            }
            // reset total energy
            meter.total_energy_wh = 0.0f;
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}