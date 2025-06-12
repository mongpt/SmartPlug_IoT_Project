#ifndef SMART_PLUG_PROJECT_RTC_TASK_H
#define SMART_PLUG_PROJECT_RTC_TASK_H

#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "RTC.h"
#include "semphr.h"
#include "queue.h"
#include "event_groups.h"
#include "common.h"

void rtc_task(void *pvParameters);

#endif // SMART_PLUG_PROJECT_RTC_TASK_H
