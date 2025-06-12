#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include <cstdlib>
#include <cstring>
#include <hardware/spi.h>
#include <cstdio>
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "display_task.h"
#include "adc_task.h"
#include "gpio_output_task.h"
#include "gpio_input_task.h"
#include "internet_task.h"
#include "eeprom_task.h"
#include "buzzer_task.h"
#include "rtc_task.h"
#include "common.h"
#include "heap_task.h"
#include "ota_task.h"


using namespace std;

extern "C" {
uint32_t read_runtime_ctr(void) {
    return timer_hw->timerawl;
}
}

PicoI2C *i2cbus = new PicoI2C(0, 100000);
void blink_led(void *pvParameters);
int main() {
    stdio_init_all();
    printf("Debug at Boot\n");

    static eeprom_data_s eeprom_data = {
            .wifi_data_q = xQueueCreate(1, sizeof(credential_s)),
            .setpoint_q = xQueueCreate(1, sizeof(float)),
            .time_q = xQueueCreate(5, sizeof(uint64_t)),
            .credential_changed_sem = xSemaphoreCreateBinary(),
            .wh_store_q = xQueueCreate(1, sizeof(char[20])),
            .wifi_connected_sem = xSemaphoreCreateBinary(),
            .wh_send_q = xQueueCreate(1, sizeof(char[20]))
    };

    static gpio_out_s gpio_out = {
            .dev_state_q = xQueueCreate(1, sizeof(uint8_t)),
            .output_sem = xSemaphoreCreateBinary(),
            .ocp_sem = xSemaphoreCreateBinary(),
            .flipflop_reset_sem = xSemaphoreCreateBinary(),
            .state_to_cloud_sem = xSemaphoreCreateBinary(),
            .state_to_internet_q = xQueueCreate(1, sizeof(uint8_t))
    };

    static gpio_in_s gpio_in = {
            .output_sem = gpio_out.output_sem,
            .wifi_reset_sem = xSemaphoreCreateBinary(),
            .qr_sem = xSemaphoreCreateBinary(),
            .ocp_sem = gpio_out.ocp_sem,
            .alarm_sem = xSemaphoreCreateBinary(),
            .gpio_in_q = xQueueCreate(2, sizeof(uint8_t )),
            .flipflop_reset_sem = gpio_out.flipflop_reset_sem
    };

    static internet_data_s internet_data = {
            .wifi_reset_sem = gpio_in.wifi_reset_sem,
            .credential_q = eeprom_data.wifi_data_q,
            .wifi_status_q = xQueueCreate(1, sizeof(uint8_t)),
            .dev_state_q = gpio_out.dev_state_q,
            .setpoint_q = eeprom_data.setpoint_q,
            .pw_consumed_q = xQueueCreate(1, sizeof(float)),
            .time_q = eeprom_data.time_q,
            .credential_changed_sem = eeprom_data.credential_changed_sem,
            .wh_send_q = eeprom_data.wh_send_q,
            .wifi_connected_sem = eeprom_data.wifi_connected_sem,
            .state_to_cloud_sem = gpio_out.state_to_cloud_sem,
            .state_to_internet_q = gpio_out.state_to_internet_q,
            .ota_state = xQueueCreate(1, sizeof(uint8_t)),
            .ota_sem = xSemaphoreCreateBinary()
    };

    static adc_s adc = {
            .adc_bat_q = xQueueCreate(1, sizeof(uint8_t )),
            .adc_current_q = xQueueCreate(10, sizeof(float)),
            .adc_voltage_q = xQueueCreate(10, sizeof(float)),
            .adc_power_q = xQueueCreate(10, sizeof(float)),
            .setpoint_q = eeprom_data.setpoint_q,
            .dev_state_q = gpio_out.dev_state_q,
            .ocp_sem = gpio_out.ocp_sem,
            .time_q = eeprom_data.time_q,
            .wh_send_q = eeprom_data.wh_send_q,
            .wifi_status_q = internet_data.wifi_status_q,
            .wh_store_q = eeprom_data.wh_store_q
    };

    static buzzer_s buzzer = {
            .gpio_in_q = gpio_in.gpio_in_q,
            .dev_state_q = gpio_out.dev_state_q
    };

    static display_data_s display_data = {
            .clock_data_q = xQueueCreate(1, sizeof(datetime_s )),
            .adc_bat_q = adc.adc_bat_q,
            .adc_current_q = adc.adc_current_q,
            .adc_voltage_q = adc.adc_voltage_q,
            .adc_power_q = adc.adc_power_q,
            .wifi_status_q = internet_data.wifi_status_q,
            .dev_state_q = gpio_out.dev_state_q,
            .time_q = eeprom_data.time_q,
            .setpoint_q = eeprom_data.setpoint_q,
            .qr_sem = gpio_in.qr_sem,
            .pw_consumed_q = internet_data.pw_consumed_q,
            .ota_sem = internet_data.ota_sem
    };

    static rtc_s rtc_data = {
            .display_clock_q = display_data.clock_data_q,
            .time_q = eeprom_data.time_q,
            .dev_state_q = gpio_out.dev_state_q,
            .alarm_sem = gpio_in.alarm_sem
    };
    static ota_s ota_data = {.ota_state = internet_data.ota_state};

    setpoint_event_group = xEventGroupCreate();
    time_event_group = xEventGroupCreate();
    state_event_group = xEventGroupCreate();

    // Reduce stack size where possible
    xTaskCreate(eeprom_task, "eeprom task", 512, &eeprom_data, tskIDLE_PRIORITY + 2, nullptr);
    xTaskCreate(gpio_ouput_task, "gpio_out", 256, &gpio_out, tskIDLE_PRIORITY + 3, nullptr);
    xTaskCreate(gpio_input_task, "gpio in", 256, &gpio_in, tskIDLE_PRIORITY + 4, nullptr);
    xTaskCreate(internet_task, "InternetTask", 2048, &internet_data, tskIDLE_PRIORITY + 3, nullptr); // Reduced from 1024 to 768
    xTaskCreate(adc_task, "adc_task", configMINIMAL_STACK_SIZE, &adc, tskIDLE_PRIORITY + 3, nullptr);
   xTaskCreate(buzzer_task, "buzzer_task", 256, &buzzer, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(display_task, "display_task", 1024, &display_data, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(rtc_task, "RTC task", configMINIMAL_STACK_SIZE, &rtc_data, tskIDLE_PRIORITY + 4, nullptr);
    //xTaskCreate(heap_task, "heap_task", 512, nullptr, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(vOTA, "OTA task", 9000, &ota_data, tskIDLE_PRIORITY + 1, nullptr);
    //xTaskCreate( blink_led, "blink_led", 128, nullptr, tskIDLE_PRIORITY + 1, nullptr);

    vTaskStartScheduler();
 
    while (1);  // Should never reach this
}

// blink led OTA test task
void blink_led(void *pvParameters) {
    while (true) {
        gpio_put(LED_GR, 1);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_put(LED_GR, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_put(LED_R, 1);
        vTaskDelay(pdMS_TO_TICKS(250));
        gpio_put(LED_R, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}