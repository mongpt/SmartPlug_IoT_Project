//
// Created by iamna on 14/01/2025.
//

#ifndef SMART_PLUG_RTC_H
#define SMART_PLUG_RTC_H

#include <cstdint>
#include "stdio.h"
#include <memory>
#include "PicoI2C.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "hardware/rtc.h"
#include "pico/stdlib.h"
#include "pico/util/datetime.h"
#include "queue.h"
#include "task.h"
// I2C Address of MCP79410
//#define MCP79410_ADDR 0x6F
#define RTC_ADDR 0x6F
// MCP79410 Registers
#define RTCSEC 0x00
#define RTCMIN 0x01
#define RTCHOUR 0x02
#define RTCWKDAY 0x03
#define RTCDATE 0x04
#define RTCMTH 0x05
#define RTCYEAR 0x06
#define CONTROL 0x07

// Alarm Registers
#define ALM0SEC  0x0A
#define ALM0MIN  0x0B
#define ALM0HOUR 0x0C
#define ALM0WKDAY 0x0D
#define ALM0DATE 0x0E
#define ALM0MTH 0x0F

#define I2C_WAIT_TIME 5

class RTC {
public:
    explicit RTC(PicoI2C *i2c, uint16_t device_address);
    ~RTC();
    void init();
    void set_time(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t weekday, uint8_t day, uint8_t month, uint8_t year);
    void get_time(uint8_t* seconds, uint8_t* minutes, uint8_t* hours, uint8_t* day, uint8_t* month, uint8_t* year);
    void get_time(uint8_t *minutes, uint8_t *hours);
    void set_alarm(uint8_t seconds, uint8_t minutes, uint8_t hours, uint8_t weekday, uint8_t date, uint8_t month);
    void clear_alarm();
    void enable_alarm();
    void disable_alarm();
    void clear_interrupt_flag();

    SemaphoreHandle_t i2c_mutex;

private:
    PicoI2C *i2c;
    uint16_t device_address;
    uint8_t dec_to_bcd(uint8_t dec);
    uint8_t bcd_to_dec(uint8_t bcd);
    bool irq_triggered = false;

};


#endif //SMART_PLUG_RTC_H
