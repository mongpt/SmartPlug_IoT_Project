//
// Created by ADMIN on 1/19/2025.
//

#ifndef SMART_PLUG_PROJECT_DISPLAY_TASK_H
#define SMART_PLUG_PROJECT_DISPLAY_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "st7789.h"
#include "screens.h"
#include "queue.h"
#include "common.h"

void display_task(void *pvParameters);

#endif //SMART_PLUG_PROJECT_DISPLAY_TASK_H
