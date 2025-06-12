#ifndef EEPROM_TASK_H
#define EEPROM_TASK_H

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "queue.h"
#include "EEPROM.h"
#include "event_groups.h"
#include "common.h"

#define EEPROM_ADDR 0x50
#define SSID_ADDR 0
#define PASSWORD_ADDR (SSID_ADDR + 64)  // from 64
#define SETPOINT_ADDR (PASSWORD_ADDR + 64)  // from 128
#define TIMER1_ADDR (SETPOINT_ADDR + 64)    //from 192 -> 6592
#define WH_ADDR 6656    // from 6656 -> 29696

#define BUFF_SIZE 32
#define NUM_OF_TIMER 100
#define NUM_WH_SLOT 360

void eeprom_task(void *pvParameters);

#endif // EEPROM_TASK_H
