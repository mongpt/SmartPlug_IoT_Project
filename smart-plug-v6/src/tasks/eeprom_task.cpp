//
// Created by ADMIN on 2/7/2025.
//

#include "eeprom_task.h"
#include <memory>
#include <PicoI2C.h>

//#define FRESH_BOARD

extern PicoI2C *i2cbus;  // Shared I2C instance

// Compare function for sorting uint64_t values
static int compare_uint64(const void *a, const void *b) {
    uint64_t num1 = *(const uint64_t *)a;
    uint64_t num2 = *(const uint64_t *)b;
    return (num1 > num2) - (num1 < num2);
}

void eeprom_task(void *pvParameters) {
    auto *data = static_cast<eeprom_data_s *>(pvParameters);
    EEPROM eeprom(i2cbus, EEPROM_ADDR);

    float setpoint = 0.0f;
    char wh_data[20] = {0};

    credential_s credential = {._ssid = {0}, ._password = {0}};
    EventBits_t time_bits;
    EventBits_t setpoint_bits;
    EventBits_t state_bits;

    // Write the default timer data to EEPROM for a fresh board
#ifdef FRESH_BOARD
    // write default wifi to EEPROM
    strcpy(credential._ssid, "my_ssid");
    strcpy(credential._password, "my_password");
    eeprom.writeToMemory(SSID_ADDR, reinterpret_cast<const uint8_t *>(credential._ssid), BUFF_SIZE);
    eeprom.writeToMemory(PASSWORD_ADDR, reinterpret_cast<const uint8_t *>(credential._password), BUFF_SIZE);
    // write default setpoint to EEPROM
    eeprom.writeFloat(SETPOINT_ADDR, 0.0f);
    // write default timer to EEPROM
    for (uint16_t i = 0; i <= NUM_OF_TIMER; i++) {
        eeprom.writeUint64ToEEPROM(TIMER1_ADDR + i * 64, 99123123592);
    }
    // write default wh to EEPROM
    char wh_default[20] = "wh";
    for (uint16_t i = 0; i < NUM_WH_SLOT; i++) {
        eeprom.writeToMemory(WH_ADDR + i * 64, reinterpret_cast<const uint8_t *>(wh_default), 20);
    }
#endif

    vTaskDelay(pdMS_TO_TICKS(2000));  // Delay for 1 second before starting

    // Read the EEPROM at boot
    // Read SSID
    eeprom.readFromMemory(SSID_ADDR, reinterpret_cast<uint8_t *>(credential._ssid), BUFF_SIZE);
    if (strlen(credential._ssid) > 0) {
        printf("SSID: %s\n", credential._ssid);

        eeprom.readFromMemory(PASSWORD_ADDR, reinterpret_cast<uint8_t *>(credential._password), BUFF_SIZE);
        printf("Password: %s\n", credential._password);

        // Send credentials to internet task
        xQueueSend(data->wifi_data_q, &credential, portMAX_DELAY);
        printf("Credentials sent to internet task\n");
    } else {
        // Write default credentials to EEPROM
        strcpy(credential._ssid, "Rhod");
        eeprom.writeToMemory(SSID_ADDR, reinterpret_cast<const uint8_t *>(credential._ssid), BUFF_SIZE);

        strcpy(credential._password, "0413113368");
        eeprom.writeToMemory(PASSWORD_ADDR, reinterpret_cast<const uint8_t *>(credential._password), BUFF_SIZE);
    }

    // Read power setpoint from EEPROM
    setpoint = eeprom.readFloat(SETPOINT_ADDR);
    // empty the queue before adding new setpoint
    xQueueReset(data->setpoint_q);
    // send the new setpoint to a queue
    xQueueSend(data->setpoint_q, &setpoint, portMAX_DELAY);
    //notify display and adc task
    xEventGroupSetBits(setpoint_event_group, SETPOINT_DISPLAY_BIT | SETPOINT_ADC_BIT);
    // Request current time from RTC task
    xEventGroupSetBits(time_event_group, TIME_EEPROM_ASK_BIT);
    // Wait for response
    time_bits = xEventGroupWaitBits(time_event_group, TIME_RTC_RESPOND_BIT,
                                    pdTRUE, pdFALSE, portMAX_DELAY);
    if (time_bits & TIME_RTC_RESPOND_BIT) {
        uint64_t received_time = 0;
        xQueueReceive(data->time_q, &received_time, portMAX_DELAY);
        printf("Received RTC time: %llu\n", received_time);
        // add ON state to the time to make a reference time
        uint64_t received_time_ON = received_time * 10 + 1;
        // Read and validate timers in EEPROM
        uint64_t list_timers[NUM_OF_TIMER] = {0};
        uint8_t list_changed = 0;
        uint8_t count_valid = 0;    // count how many valid timers in eeprom
        for (uint8_t i = 0; i < NUM_OF_TIMER; i++) {
            list_timers[i] = eeprom.readUint64FromEEPROM(TIMER1_ADDR + i * 64);
            if (list_timers[i] == 99123123592) {    // stop reading eeprom when found an invalid timer
                break;
            }
            count_valid++;  // increment the count since a valid timer is found
            if (list_timers[i] <= received_time_ON) {   // Check if timer is in the past
                list_timers[i] = 99123123592;  // Set to out-of-range time
                list_changed = 1;   // Set flag to write back to EEPROM
            }
        }
        if (list_changed) {
            if (count_valid > 1) {  // Sort only if there are more than 1 timer
                // Sort the list
                qsort(list_timers, count_valid, sizeof(uint64_t), compare_uint64);
            }
            // write back to EEPROM
            for (uint8_t i = 0; i < count_valid; i++) {
                eeprom.writeUint64ToEEPROM(TIMER1_ADDR + i * 64, list_timers[i]);
            }
        }
        // empty the queue before adding new timers
        xQueueReset(data->time_q);
        // send the first timer to rtc and display task
        xQueueSend(data->time_q, &list_timers[0], portMAX_DELAY);
        // Notify RTC and display task
        xEventGroupSetBits(time_event_group, TIME_SET_ALARM_BIT | TIME_DISPLAY_BIT);
    }

    while (true) {

        // Handle new power setpoint
        setpoint_bits = xEventGroupWaitBits(setpoint_event_group, SETPOINT_EEPROM_BIT,
                                               pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (setpoint_bits & SETPOINT_EEPROM_BIT) {
            if (xQueuePeek(data->setpoint_q, &setpoint, pdMS_TO_TICKS(0)) == pdTRUE) {
                // update EEPROM
                eeprom.writeFloat(SETPOINT_ADDR, setpoint);
            }
        }

        // Handle new timer data
        time_bits = xEventGroupWaitBits(time_event_group, TIME_EEPROM_INTERNET_BIT,
                                   pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (time_bits & TIME_EEPROM_INTERNET_BIT) {
            uint64_t timer = 0;
            if (xQueueReceive(data->time_q, &timer, pdMS_TO_TICKS(0)) == pdTRUE) {
                // read all timers in EEPROM
                uint64_t list_timers[NUM_OF_TIMER+1] = {0};
                uint8_t count = 0;    // count how many valid timers in eeprom
                for (count = 0; count <= NUM_OF_TIMER; count++) {
                    list_timers[count] = eeprom.readUint64FromEEPROM(TIMER1_ADDR + count * 64);
                    if (list_timers[count] == 99123123592) {    // stop reading eeprom when found an invalid timer
                        break;
                    }
                }
                // after above for{}, count increased by 1
                list_timers[count] = timer;
                count++;  // increment the count since a valid timer is added
                if (count > 1) {  // Sort only if there are more than 1 timer
                    // Sort the list
                    qsort(list_timers, count, sizeof(uint64_t), compare_uint64);
                }
                // write back to EEPROM (only the first <= NUM_OF_TIMER timers)
                count = (count > NUM_OF_TIMER) ? NUM_OF_TIMER : count;
                for (uint8_t i = 0; i < count; i++) {
                    eeprom.writeUint64ToEEPROM(TIMER1_ADDR + i * 64, list_timers[i]);
                }
                // empty the queue before adding new timer
                xQueueReset(data->time_q);
                // send the first 2 timers to queue for rtc and display task
                xQueueSend(data->time_q, &list_timers[0], portMAX_DELAY);
                // Notify RTC and display task
                xEventGroupSetBits(time_event_group, TIME_SET_ALARM_BIT | TIME_DISPLAY_BIT);
            }
        }

        // check if timer has been triggered by RTC task
        state_bits = xEventGroupWaitBits(state_event_group, STATE_EEPROM_BIT,
                                   pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (state_bits & STATE_EEPROM_BIT) {
            // Read timers in EEPROM
            uint64_t list_timers[NUM_OF_TIMER] = {0};
            uint8_t count_valid = 0;    // count how many valid timers in eeprom
            for (uint8_t i = 0; i < NUM_OF_TIMER; i++) {
                list_timers[i] = eeprom.readUint64FromEEPROM(TIMER1_ADDR + i * 64);
                if (list_timers[i] == 99123123592) {    // stop reading eeprom when found an invalid timer
                    break;
                }
                count_valid++;  // increment the count since a valid timer is found
            }
            // Shift the list to remove the first timer
            for (uint8_t i = 0; i < count_valid - 1; i++) {  // Shift the list to the left by 1
                list_timers[i] = list_timers[i + 1];
            }
            // Clear the last item (Set to out-of-range time)
            list_timers[count_valid - 1] = 99123123592;
            // write back to EEPROM
            for (uint8_t i = 0; i < count_valid; i++) {
                eeprom.writeUint64ToEEPROM(TIMER1_ADDR + i * 64, list_timers[i]);
            }
            // send new timer to queue
            // empty the queue before adding new timers
            xQueueReset(data->time_q);
            // send the first timer to rtc and display task
            xQueueSend(data->time_q, &list_timers[0], portMAX_DELAY);
            // Notify display task first
            xEventGroupSetBits(time_event_group, TIME_DISPLAY_BIT);

            // only send the timer to RTC task if it is valid
            if (list_timers[0] != 99123123592) {
                xEventGroupSetBits(time_event_group, TIME_SET_ALARM_BIT);
            }
        }

        // check if new wifi credentials are set
        if (xSemaphoreTake(data->credential_changed_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
            // read the new credentials
            xQueueReceive(data->wifi_data_q, &credential, pdMS_TO_TICKS(0));
            // write to EEPROM
            eeprom.writeToMemory(SSID_ADDR, reinterpret_cast<const uint8_t *>(credential._ssid), BUFF_SIZE);
            eeprom.writeToMemory(PASSWORD_ADDR, reinterpret_cast<const uint8_t *>(credential._password), BUFF_SIZE);
        }

        //check if wh data needs to be stored
        if (xQueueReceive(data->wh_store_q, &wh_data, pdMS_TO_TICKS(0)) == pdTRUE) {
            printf("Received wh data to be stored: %s\n", wh_data);
            // check available slot in EEPROM
            char tmp[20] = {0};
            for (uint16_t i = 0; i < NUM_WH_SLOT; i++) {
                eeprom.readFromMemory(WH_ADDR + i * 64, reinterpret_cast<uint8_t *>(tmp), 20);
                if (strcmp(tmp, "wh") == 0) {   // found an empty slot
                    eeprom.writeToMemory(WH_ADDR + i * 64, reinterpret_cast<const uint8_t *>(wh_data), 20);
                    break;
                }
            }
        }

        //check if wifi is back online to send wh data
        if (xSemaphoreTake(data->wifi_connected_sem, pdMS_TO_TICKS(0)) == pdTRUE) {
            // check available slot in EEPROM
            char tmp[20] = {0};
            for (uint16_t i = 0; i < NUM_WH_SLOT; i++) {
                eeprom.readFromMemory(WH_ADDR + i * 64, reinterpret_cast<uint8_t *>(tmp), 20);
                if (strcmp(tmp, "wh") != 0) {   // found a valid slot
                    // send data to cloud
                    xQueueSend(data->wh_send_q, &tmp, portMAX_DELAY);
                    // clear the slot
                    strcpy(tmp, "wh");
                    eeprom.writeToMemory(WH_ADDR + i * 64, reinterpret_cast<const uint8_t *>(tmp), 20);
                } else break;
            }
        }

        // Check if a timer needs to be removed
        time_bits = xEventGroupWaitBits(time_event_group, TIME_REMOVE_EEPROM_INTERNET_BIT,
                                        pdTRUE, pdFALSE, pdMS_TO_TICKS(0));
        if (time_bits & TIME_REMOVE_EEPROM_INTERNET_BIT) {
            uint64_t timer = 0;
            if (xQueueReceive(data->time_q, &timer, pdMS_TO_TICKS(0)) == pdTRUE) {
                // read all timers in EEPROM
                uint64_t list_timers[NUM_OF_TIMER] = {0};
                uint8_t count = 0;    // count how many valid timers in eeprom
                for (count = 0; count < NUM_OF_TIMER; count++) {
                    list_timers[count] = eeprom.readUint64FromEEPROM(TIMER1_ADDR + count * 64);
                    if (list_timers[count] == 99123123592) {    // stop reading eeprom when found an invalid timer
                        break;
                    } else if (list_timers[count] == timer) {   // find a matched timer
                        list_timers[count] = 99123123592;   // remove old timer by assigning invalid number
                    }
                }
                // after above for{}, count increased by 1
                if (count > 1) {  // Sort only if there are more than 1 timer
                    // Sort the list
                    qsort(list_timers, count, sizeof(uint64_t), compare_uint64);
                }
                // write back to EEPROM (only the first <= NUM_OF_TIMER timers)
                count = (count > NUM_OF_TIMER) ? NUM_OF_TIMER : count;
                for (uint8_t i = 0; i < count; i++) {
                    eeprom.writeUint64ToEEPROM(TIMER1_ADDR + i * 64, list_timers[i]);
                }
                // empty the queue before adding new timer
                xQueueReset(data->time_q);
                // send the first 2 timers to queue for rtc and display task
                xQueueSend(data->time_q, &list_timers[0], portMAX_DELAY);
                // Notify RTC and display task
                xEventGroupSetBits(time_event_group, TIME_SET_ALARM_BIT | TIME_DISPLAY_BIT);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
