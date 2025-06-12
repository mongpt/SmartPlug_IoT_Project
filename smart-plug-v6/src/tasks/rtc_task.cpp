//
// Created by ADMIN on 2/20/2025.
//

#include "rtc_task.h"

extern PicoI2C *i2cbus;

// Calculate weekday using Zeller's Congruence
uint8_t get_weekday(uint8_t day, uint8_t month, uint16_t year) {
    if (month < 3) { month += 12; year--; }
    int K = year % 100, J = year / 100;
    return (((day + (13 * (month + 1)) / 5 + K + (K / 4) + (J / 4) - (2 * J)) % 7) + 5) % 7 + 1;
}

uint64_t get_and_pack_time(RTC &rtc) {
    datetime_s datetime;
    // read current time
    rtc.get_time(&datetime.seconds, &datetime.minutes, &datetime.hours,
                 &datetime.day, &datetime.month, &datetime.year);
    printf("Current Time: %u:%u:%u, Date: %u/%u/%u\n",
           datetime.hours, datetime.minutes, datetime.seconds, datetime.day, datetime.month, datetime.year);
    // pack time into a single uint64_t
    return
            (uint64_t)datetime.year * 100000000ULL +
            (uint64_t)datetime.month * 1000000ULL +
            (uint64_t)datetime.day * 10000ULL +
            (uint64_t)datetime.hours * 100ULL +
            (uint64_t)datetime.minutes;
}

void rtc_task(void *pvParameters) {
    auto *data = static_cast<rtc_s *>(pvParameters);

    uint64_t rtc_time = 0;
    datetime_s datetime;
    time_only_s current_time, clock;

    uint8_t previous_minute = 0, weekday = 0, state = 0, send_wh = 0;
    uint64_t packed_time = 0;

    EventBits_t time_bits;

//    setup_alarm_interrupt();
    RTC rtc(i2cbus, RTC_ADDR);

//    weekday = get_weekday(27, 2, 2025);
//    printf("Calculated Weekday: %d\n", weekday);
//    rtc.set_time(30, 53, 9, weekday, 27, 2, 25);
    printf("RTC Initialized\n");
//    rtc.set_alarm(0,54,9,weekday,27,2);

    while (true) {
        // Handle alarm event
        if (xSemaphoreTake(data->alarm_sem, pdMS_TO_TICKS(10)) == pdTRUE) {
            printf("ALARM TRIGGERED!\n");
            rtc.clear_alarm();
            // notify output, display, eeprom, and buzzer task
            xQueueReset(data->dev_state_q);
            xQueueSend(data->dev_state_q, &state, portMAX_DELAY);
            xEventGroupSetBits(state_event_group,
                               STATE_OUTPUT_BIT | STATE_DISPLAY_BIT | STATE_BUZZER_BIT | STATE_EEPROM_BIT);
        }

        // Check if new time needs to be updated
        time_bits = xEventGroupWaitBits(time_event_group, TIME_UPDATE_TIME_BIT,
                                               pdFALSE, pdFALSE, pdMS_TO_TICKS(10));
        if (time_bits & TIME_UPDATE_TIME_BIT) {
            if (xQueuePeek(data->time_q, &rtc_time, pdMS_TO_TICKS(10)) == pdTRUE) {
                // clear TIME_RTC_BIT
                xEventGroupClearBits(time_event_group, TIME_UPDATE_TIME_BIT);
                // unpack time
                printf("Received Packed Time: %llu\n", rtc_time);
                datetime.seconds = rtc_time % 100ULL;
                datetime.minutes = (rtc_time / 100ULL) % 100ULL;
                datetime.hours = (rtc_time / 10000ULL) % 100ULL;
                datetime.day = (rtc_time / 1000000ULL) % 100ULL;
                datetime.month = (rtc_time / 100000000ULL) % 100ULL;
                datetime.year = rtc_time / 10000000000ULL;
                // calculate weekday
                weekday = get_weekday(datetime.day, datetime.month, datetime.year + 2000);
                printf("Calculated Weekday: %d\n", weekday);
                // update RTC
                rtc.set_time(datetime.seconds, datetime.minutes, datetime.hours, weekday, datetime.day, datetime.month, datetime.year);
                printf("Updated Time: %02u:%02u:%02u, Date: %02u/%02u/20%02u\n",
                       datetime.hours, datetime.minutes, datetime.seconds, datetime.day, datetime.month, datetime.year);
            }
        }

        // Check if EEPROM requests current time
        time_bits = xEventGroupWaitBits(time_event_group, TIME_EEPROM_ASK_BIT,
                                               pdFALSE, pdFALSE, pdMS_TO_TICKS(10));
        if (time_bits & TIME_EEPROM_ASK_BIT) {
            // read current time and pack it
            packed_time = get_and_pack_time(rtc);
            printf("Packed Time: %llu\n", packed_time);
            // send time to EEPROM
            xQueueReset(data->time_q);
            xQueueSend(data->time_q, &packed_time, portMAX_DELAY);
            // clear TIME_EEPROM_BIT
            xEventGroupClearBits(time_event_group, TIME_EEPROM_ASK_BIT);
            // notify EEPROM task
            xEventGroupSetBits(time_event_group, TIME_RTC_RESPOND_BIT);
            printf("Sent Time to EEPROM\n");
        }

        // Check if EEPROM sent a new timer
        time_bits = xEventGroupWaitBits(time_event_group, TIME_SET_ALARM_BIT,
                                   pdFALSE, pdFALSE, pdMS_TO_TICKS(10));
        if (time_bits & TIME_SET_ALARM_BIT) {
            if (xQueuePeek(data->time_q, &packed_time, pdMS_TO_TICKS(10)) == pdTRUE) {
                // clear TIME_SET_ALARM_BIT
                xEventGroupClearBits(time_event_group, TIME_SET_ALARM_BIT);
                // unpack time
                state = packed_time % 10;
                datetime.seconds = 0;
                datetime.minutes = (packed_time / 10ULL) % 100ULL;
                datetime.hours = (packed_time / 1000ULL) % 100ULL;
                datetime.day = (packed_time / 100000ULL) % 100ULL;
                datetime.month = (packed_time / 10000000ULL) % 100ULL;
                datetime.year = packed_time / 1000000000ULL;
                weekday = get_weekday(datetime.day, datetime.month, datetime.year + 2000);
                // set alarm
                rtc.set_alarm(datetime.seconds, datetime.minutes, datetime.hours,
                              weekday, datetime.day, datetime.month);
                printf("Alarm Set: %02u:%02u, Date: %02u/%02u\n", datetime.hours,
                       datetime.minutes, datetime.day, datetime.month);
            }
        }

        // Get current time and update display
        rtc.get_time(&current_time.minute, &current_time.hour);
        if (current_time.minute != previous_minute) {
            clock.hour = current_time.hour;
            clock.minute = current_time.minute;
            xQueueSend(data->display_clock_q, &clock, pdMS_TO_TICKS(100));
            previous_minute = clock.minute;
            printf("Current Time: %02u:%02u\n", clock.hour, clock.minute);
        }
        // check if hour is just changed by checking the minute = 0
        if (current_time.minute % 5 == 0) {
            if (send_wh == 0) {
                send_wh = 1;
                // read current time and pack it
                packed_time = get_and_pack_time(rtc);
                printf("Packed Time: %llu\n", packed_time);
                // send time to ADC task
                xQueueReset(data->time_q);
                xQueueSend(data->time_q, &packed_time, portMAX_DELAY);
                // notify ADC task
                xEventGroupSetBits(time_event_group, TIME_TO_SEND_WH_ADC_BIT);
            }
        } else send_wh = 0;

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
