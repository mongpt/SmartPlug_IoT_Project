#ifndef SMART_PLUG_PROJECT_SCREENS_H
#define SMART_PLUG_PROJECT_SCREENS_H

#include "st7789.h"
#include "qrcode_st7789.h"

#define START_COL_POS 8

class SCREEN {
public:
    explicit SCREEN(ST7789 *lcd): lcd_(lcd){}
    void showFrame() const;
    void showClock(uint8_t h, uint8_t m) const;
    void showState(uint8_t s) const;
    void showTimer(const std::string &timer, uint8_t state) const;
    void showVoltage(float V) const;
    void showCurrent(float I) const;
    void showPower(float P) const;
    void showBAT(uint8_t p) const;
    void showWIFI(uint8_t s) const;
    void showSetPw(float P) const;
    void showQR(uint8_t num) const;
    void showConsumption(float wh) const;
    void showOTA() const;

private:
    ST7789 *lcd_;

};

#endif //SMART_PLUG_PROJECT_SCREENS_H
