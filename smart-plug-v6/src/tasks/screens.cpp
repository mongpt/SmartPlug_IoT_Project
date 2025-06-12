#include <cstdio>
#include <string>
#include "screens.h"

void SCREEN::showFrame() const {
    lcd_->enableFramebuffer(false);
    lcd_->fill(ST77XX_BLACK);
    uint16_t color = ST77XX_WHITE;
    uint16_t w = lcd_->width_;
    uint16_t h = lcd_->height_;
    lcd_->drawRoundRect(0, 0, w, h, 5, color);
    lcd_->enableFramebuffer(true);
}

void SCREEN::showClock(const uint8_t h, const uint8_t m) const {
    uint8_t txtSize = 3;
    char clock[6] = "\0";
    char hr[3], mn[3];
    sprintf(hr, "%02d", h);
    sprintf(mn, "%02d", m);
    lcd_->cursor_x = 145;
    lcd_->cursor_y = 50;
    lcd_->setWindow(lcd_->cursor_x-6, lcd_->cursor_y-17, lcd_->width_- 1, (lcd_->cursor_y-17)+58);
    lcd_->fillRect(lcd_->cursor_x-6, lcd_->cursor_y-17, 100, 58, ST77XX_ORANGE);
    lcd_->setTextColor(ST77XX_BLUE);
    lcd_->setTextSize(txtSize);
    sprintf(clock, "%s:%s", hr, mn);
    lcd_->print(clock);
    lcd_->flushRegion();
}

void SCREEN::showState(const uint8_t s) const {
    char text[13];
    uint16_t color = ST77XX_BLACK;
    if (s == 0) {
        sprintf(text, "Socket:OFF");
        color = ST77XX_RED;
    } else if (s == 1) {
        sprintf(text, "Socket:ON");
        color = ST77XX_GREEN;
    } else if (s == 2) {
        sprintf(text, "Socket:OL");
        color = ST77XX_ORANGE;
    }
    uint8_t txtSize = 2;
    lcd_->cursor_x = START_COL_POS;
    lcd_->cursor_y = 9;
    lcd_->setWindow(lcd_->cursor_x, lcd_->cursor_y, lcd_->cursor_x+125, lcd_->cursor_y+DEFAULT_TEXT_H * txtSize + 1);
    lcd_->fillRect(lcd_->cursor_x, lcd_->cursor_y, 125, DEFAULT_TEXT_H * txtSize + 1, ST77XX_BLACK);

    lcd_->setTextColor(color);
    lcd_->setTextSize(txtSize);
    lcd_->print(text);
    lcd_->cursor_x = START_COL_POS + 1;
    lcd_->cursor_y = 10;
    lcd_->print(text);
    lcd_->flushRegion();
}

void SCREEN::showBAT(const uint8_t p) const {
    uint16_t color = p > 10 ? ST77XX_GREEN : ST77XX_RED;
    uint8_t n_char = 0;
    const uint8_t txt_size = 2;
    const int16_t y = 5;
    const int16_t w = 6;
    const int16_t h = 4;
    if (p < 10.0) n_char = 1;
    else if (p < 100.0) n_char = 2;
    else n_char = 3;
    uint16_t n_space = w + 6 + (n_char + 1) * txt_size * DEFAULT_TEXT_W;
    int16_t x = 130 + (210 - 130 - n_space) / 2;

    lcd_->setWindow(130, y, 130+210-135, y+y+h+14);
    lcd_->fillRect(130, y, 210-135, y+h+14, ST77XX_BLACK);
    lcd_->fillRoundRect(x, y, w, h, 1, color);
    lcd_->fillRect(x-3, y+h-1, w+6, 2, color);
    lcd_->drawRect(x-3, y+h+1, w+6, 9, color);
    lcd_->fillRect(x-3, y+h+9, w+6, 9, color);
    lcd_->cursor_x = x+12;
    lcd_->cursor_y = 9;
    lcd_->setTextColor(color);
    lcd_->setTextSize(txt_size);
    lcd_->printFloat(p, 0);
    lcd_->print("%");
    lcd_->flushRegion();
}

void SCREEN::showWIFI(const uint8_t s) const {
    const uint16_t x = 210;
    uint16_t y = 25;
    const uint16_t r_start = 6;
    const uint16_t r_increment = 6;
    uint16_t color = s ? ST77XX_GREEN : ST77XX_RED;

    lcd_->setWindow(x, 5, lcd_->width_- 1, 5+28);
    for (uint8_t j = 0; j < 2; j++) {
        for (uint8_t i = 0; i < 3; i++) {
            uint16_t r_outer = r_start + i * r_increment;
            lcd_->drawCircleHelper(x-j, y+j, r_outer, 0b10, color);
        }
    }
    lcd_->fillCircle(x, y, 1, color);
    lcd_->flushRegion();
}

void SCREEN::showVoltage(const float V) const {
    uint8_t txtSize = 2;
    uint16_t color = ST77XX_BLUE;
    lcd_->cursor_x = START_COL_POS;
    lcd_->cursor_y = 147;
    lcd_->setTextColor(color);
    lcd_->setTextSize(txtSize);
    lcd_->setWindow(lcd_->cursor_x, lcd_->cursor_y, lcd_->cursor_x+112, lcd_->cursor_y+txtSize * DEFAULT_TEXT_H + 2);
    lcd_->fillRect(lcd_->cursor_x, lcd_->cursor_y, 112, txtSize * DEFAULT_TEXT_H + 2, ST77XX_BLACK);
    lcd_->print("Vol:");
    lcd_->printFloat(V, 1);
    lcd_->print("V");
    lcd_->flushRegion();
}

void SCREEN::showCurrent(const float I) const {
    uint8_t txtSize = 2;
    uint16_t color = ST77XX_BLUE;
    lcd_->cursor_x = START_COL_POS;
    lcd_->cursor_y = 167;
    lcd_->setTextColor(color);
    lcd_->setTextSize(txtSize);
    lcd_->setWindow(lcd_->cursor_x, lcd_->cursor_y, lcd_->cursor_x+112, lcd_->cursor_y+txtSize * DEFAULT_TEXT_H + 2);
    lcd_->fillRect(lcd_->cursor_x, lcd_->cursor_y, 112, txtSize * DEFAULT_TEXT_H + 2, ST77XX_BLACK);
    lcd_->print("Cur:");
    lcd_->printFloat(I, 1);
    lcd_->print("A");
    lcd_->flushRegion();
}

void SCREEN::showPower(const float P) const {
    uint8_t txtSize = 2;
    uint16_t color = ST77XX_ORANGE;
    lcd_->cursor_x = START_COL_POS;
    lcd_->cursor_y = 190;
    lcd_->setTextColor(color);
    lcd_->setTextSize(txtSize);
    lcd_->setWindow(lcd_->cursor_x, lcd_->cursor_y, lcd_->cursor_x+112, lcd_->cursor_y+txtSize * DEFAULT_TEXT_H + 2);
    lcd_->fillRect(lcd_->cursor_x, lcd_->cursor_y, 112, txtSize * DEFAULT_TEXT_H + 2, ST77XX_BLACK);
    lcd_->print("Po:");
    lcd_->printFloat(P, 1);
    lcd_->print("W");
    lcd_->flushRegion();
}

void SCREEN::showSetPw(const float P) const {
    uint8_t txtSize = 2;
    lcd_->cursor_x = START_COL_POS;
    lcd_->cursor_y = 222;
    lcd_->setWindow(lcd_->cursor_x, lcd_->cursor_y, lcd_->cursor_x+112, lcd_->cursor_y+DEFAULT_TEXT_H * txtSize);
    lcd_->fillRect(lcd_->cursor_x, lcd_->cursor_y, 112, DEFAULT_TEXT_H * txtSize, ST77XX_BLACK);
    lcd_->setTextColor(ST77XX_MAGENTA);
    lcd_->setTextSize(txtSize);
    lcd_->print("SP:");
    lcd_->printFloat(P, 1);
    lcd_->print("W");
    lcd_->flushRegion();
}

void SCREEN::showQR(const uint8_t num) const {
    char data[50] = {0};
    char label[6] = {0};
    switch (num) {
        case 0:
            memcpy(data, "WIFI:S:SmartPlugWiFi;T:WPA;P:password;H:false;;", 48);
            memcpy(label, "D WiFi", 6);
            break;
        case 1:
            memcpy(data, "http://192.168.4.1", 19);
            memcpy(label, "H WiFi", 6);
            break;
        case 2:
            memcpy(data, "www.google.com", 15);
            memcpy(label, "Manage", 6);
            break;
        default:
            break;
    }

    lcd_->cursor_x = 123;
    lcd_->cursor_y = 144;
    lcd_->setWindow(lcd_->cursor_x, lcd_->cursor_y, lcd_->width_-1, lcd_->height_-1);

    QRcode_ST7789 qrcode (lcd_);
    qrcode.init(90, 145, 147);
    qrcode.create(data);

    lcd_->setTextColor(ST77XX_WHITE);
    lcd_->setTextSize(2);
    lcd_->fillRect(lcd_->cursor_x, lcd_->cursor_y, 18, 94, ST77XX_BLUE);

    lcd_->cursor_x = 127;
    lcd_->cursor_y = 145;
    lcd_->printChar(label[0]);

    lcd_->cursor_x = 127;
    lcd_->cursor_y = 158;
    lcd_->printChar(label[1]);

    lcd_->cursor_x = 127;
    lcd_->cursor_y = 173;
    lcd_->printChar(label[2]);

    lcd_->cursor_x = 127;
    lcd_->cursor_y = 189;
    lcd_->printChar(label[3]);

    lcd_->cursor_x = 127;
    lcd_->cursor_y = 205;
    lcd_->printChar(label[4]);

    lcd_->cursor_x = 127;
    lcd_->cursor_y = 221;
    lcd_->printChar(label[5]);
    lcd_->flushRegion();
}

void SCREEN::showTimer(const std::string &timer, const uint8_t state) const{
    uint8_t txtSize = 2;
    char state_str[4] = "\0";
    lcd_->setTextSize(txtSize);
    lcd_->cursor_x = START_COL_POS + 52;
    lcd_->cursor_y = 97;
    lcd_->setWindow(1, lcd_->cursor_y - 6, 1+WIDTH - 2, lcd_->cursor_y - 6+50);
    lcd_->fillRect(1, lcd_->cursor_y - 6, WIDTH - 2, 50, ST77XX_DARKGREY);
    lcd_->setTextColor(ST77XX_YELLOW);
    lcd_->print("Scheduler:");

    memcpy(state_str, state ? "ON" : "OFF", 4);
    lcd_->cursor_x = START_COL_POS + 15;
    lcd_->cursor_y = 120;
    uint16_t color = state ? ST77XX_GREEN : ST77XX_RED;
    lcd_->setTextColor(color);
    lcd_->print(timer.c_str());
    lcd_->print(" > ");
    lcd_->print(state_str);
    lcd_->flushRegion();
}

void SCREEN::showConsumption(const float wh) const {
    uint8_t digit;
    uint8_t intPart = static_cast<uint8_t >(wh); // Extract integer part (100)
    uint8_t fractionalPart = static_cast<uint8_t >(round((wh - intPart) * 100));
    if (fractionalPart == 0) digit = 0;
    else if ((fractionalPart % 10) == 0) digit = 1;
    else digit = 2;
    uint8_t txtSize = 3;
    lcd_->cursor_x = START_COL_POS;
    lcd_->cursor_y = 40;
    lcd_->setWindow(1, lcd_->cursor_y - 7, 1+137, lcd_->cursor_y - 7+58);
    lcd_->fillRect(1, lcd_->cursor_y - 7, 137, 58, ST77XX_BLUE);
    lcd_->setTextColor(ST77XX_WHITE);
    lcd_->setTextSize(txtSize);
    lcd_->printFloat(wh, digit);

    lcd_->cursor_x = START_COL_POS;
    lcd_->cursor_y = 70;
    txtSize = 2;
    lcd_->setTextSize(txtSize);
    lcd_->print("(Wh)");
    lcd_->flushRegion();
}

void SCREEN::showOTA() const {
    lcd_->enableFramebuffer(false);
    lcd_->fill(ST77XX_BLACK);
    uint16_t color = ST77XX_WHITE;
    uint16_t w = lcd_->width_;
    uint16_t h = lcd_->height_;

    lcd_->cursor_x = START_COL_POS;
    lcd_->cursor_y = 120;
    lcd_->setTextColor(color);
    lcd_->setTextSize(2);
    lcd_->print("OTA Updating...");
}