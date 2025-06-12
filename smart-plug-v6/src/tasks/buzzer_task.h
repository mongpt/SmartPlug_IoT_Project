//
// Created by ADMIN on 2/15/2025.
//

#ifndef SMART_PLUG_PROJECT_BUZZER_TASK_H
#define SMART_PLUG_PROJECT_BUZZER_TASK_H

#include "FreeRTOS.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "common.h"

#define BUZZER 17

void buzzer_task(void *pvParameters);

#endif //SMART_PLUG_PROJECT_BUZZER_TASK_H
