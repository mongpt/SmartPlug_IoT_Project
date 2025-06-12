//
// Created by ADMIN on 1/20/2025.
//

#ifndef ADC_TASK_H
#define ADC_TASK_H

#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include <algorithm>
#include <cstdio>
#include "queue.h"
#include "common.h"

#define I_SENSE 26
#define VBAT 27
#define LOAD_VOL 28
#define ADC_VREF 3.305f  // Reference voltage in volts
#define ADC_RESOLUTION 4095  // 12-bit ADC resolution (2^12 - 1)
#define ADC_LOAD_CURRENT_CHANNEL 0
#define ADC_RTC_BAT_CHANNEL 1
#define ADC_LOAD_VOLTAGE_CHANNEL 2
#define MIN_VBAT 1.6f
#define MAX_VBAT 3.0f
#define NUM_SAMPLES 10
#define MAX_CURRENT 4.5f    // Maximum current allowed for the device

static double map(double adc_vol, double min_vol, double max_vol);
void adc_task(void *pvParameters);

#endif //ADC_TASK_H
