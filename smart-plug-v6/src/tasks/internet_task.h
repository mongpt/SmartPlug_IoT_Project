#ifndef INTERNET_TASK_H
#define INTERNET_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "NetworkClass.h"
#include "MQTTClient.h"
#include "common.h"

void internet_task(void *pvParameters);

#endif // INTERNET_TASK_H
