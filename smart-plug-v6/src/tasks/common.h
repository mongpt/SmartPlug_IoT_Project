//
// Created by ADMIN on 2/24/2025.
//

#ifndef SMART_PLUG_PROJECT_COMMON_H
#define SMART_PLUG_PROJECT_COMMON_H

#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"

#define SETPOINT_EEPROM_BIT (1 << 0)
#define SETPOINT_ADC_BIT (1 << 1)
#define SETPOINT_DISPLAY_BIT (1 << 2)

#define TIME_UPDATE_TIME_BIT (1 << 0)
#define TIME_DISPLAY_BIT (1 << 1)
#define TIME_EEPROM_ASK_BIT (1 << 2)
#define TIME_RTC_RESPOND_BIT (1 << 3)
#define TIME_EEPROM_INTERNET_BIT (1 << 4)
#define TIME_SET_ALARM_BIT (1 << 5)
#define TIME_TO_SEND_WH_ADC_BIT (1 << 6)
#define TIME_REMOVE_EEPROM_INTERNET_BIT (1 << 7)

#define STATE_OUTPUT_BIT (1 << 0)
#define STATE_DISPLAY_BIT (1 << 1)
#define STATE_BUZZER_BIT (1 << 2)
#define STATE_EEPROM_BIT (1 << 3)

extern EventGroupHandle_t setpoint_event_group;    // related to power setpoint
extern EventGroupHandle_t time_event_group;        // related to time and timers
extern EventGroupHandle_t state_event_group;       // related to device state

typedef enum {
    NONE,
    AP_BTN,
    QR_BTN,
    SW_BTN,
    OCP_TRIGGER,
    ALARM_INT
} EventType;

// WiFi credentials struct
typedef struct {
    char _ssid[32];
    char _password[32];
} credential_s;

// Struct containing queues and semaphores for the internet task
typedef struct {
    SemaphoreHandle_t wifi_reset_sem;
    QueueHandle_t credential_q;
    QueueHandle_t wifi_status_q;
    QueueHandle_t dev_state_q;
    QueueHandle_t setpoint_q;
    QueueHandle_t pw_consumed_q;
    QueueHandle_t time_q;
    SemaphoreHandle_t credential_changed_sem;
    QueueHandle_t wh_send_q;
    SemaphoreHandle_t wifi_connected_sem;
    SemaphoreHandle_t state_to_cloud_sem;
    QueueHandle_t state_to_internet_q;
    QueueHandle_t ota_state; //for over the air update
    SemaphoreHandle_t ota_sem;
} internet_data_s;

// EEPROM task shared data struct
typedef struct {
    QueueHandle_t wifi_data_q;
    QueueHandle_t setpoint_q;
    QueueHandle_t time_q;
    SemaphoreHandle_t credential_changed_sem;
    QueueHandle_t wh_store_q;
    SemaphoreHandle_t wifi_connected_sem;
    QueueHandle_t wh_send_q;
} eeprom_data_s;

// Structure for RTC task data
typedef struct {
    QueueHandle_t display_clock_q;
    QueueHandle_t time_q;
    QueueHandle_t dev_state_q;
    SemaphoreHandle_t alarm_sem;
} rtc_s;

// Structure for clock data
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t day;
    uint8_t month;
    uint8_t year;
} datetime_s;

// Structure for current time updates (hour and minute only)
typedef struct {
    uint8_t hour;
    uint8_t minute;
} time_only_s;

typedef struct {
    QueueHandle_t clock_data_q;
    QueueHandle_t adc_bat_q;
    QueueHandle_t adc_current_q;
    QueueHandle_t adc_voltage_q;
    QueueHandle_t adc_power_q;
    QueueHandle_t wifi_status_q;
    QueueHandle_t dev_state_q;
    QueueHandle_t time_q;
    QueueHandle_t setpoint_q;
    SemaphoreHandle_t qr_sem;
    QueueHandle_t pw_consumed_q;
    SemaphoreHandle_t ota_sem;
} display_data_s;

typedef struct {
    QueueHandle_t adc_bat_q;
    QueueHandle_t adc_current_q;
    QueueHandle_t adc_voltage_q;
    QueueHandle_t adc_power_q;
    QueueHandle_t setpoint_q;
    QueueHandle_t dev_state_q;
    SemaphoreHandle_t ocp_sem;
    QueueHandle_t time_q;
    QueueHandle_t wh_send_q;
    QueueHandle_t wifi_status_q;
    QueueHandle_t wh_store_q;
}adc_s;

typedef struct {
    QueueHandle_t dev_state_q;
    SemaphoreHandle_t output_sem;
    SemaphoreHandle_t ocp_sem;
    SemaphoreHandle_t flipflop_reset_sem;
    SemaphoreHandle_t state_to_cloud_sem;
    QueueHandle_t state_to_internet_q;
}gpio_out_s;

typedef struct {
    SemaphoreHandle_t output_sem;
    SemaphoreHandle_t wifi_reset_sem;
    SemaphoreHandle_t qr_sem;
    SemaphoreHandle_t ocp_sem;
    SemaphoreHandle_t alarm_sem;
    QueueHandle_t gpio_in_q;
    SemaphoreHandle_t flipflop_reset_sem;
}gpio_in_s;

typedef struct {
    QueueHandle_t gpio_in_q;
    QueueHandle_t dev_state_q;
} buzzer_s;

typedef struct {
    QueueHandle_t ota_state;
} ota_s;
#endif //SMART_PLUG_PROJECT_COMMON_H
