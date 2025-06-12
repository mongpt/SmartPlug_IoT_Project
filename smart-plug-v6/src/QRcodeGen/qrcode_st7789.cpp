
#include "qrencode.h"
#include "qrcode_st7789.h"
#include "st7789.h"

QRcode_ST7789::QRcode_ST7789(ST7789 *display):display(display) {
    //this->display(display);
}


void QRcode_ST7789::init(int width, int posX, int posY) {
    display->fillRect(posX - 2, posY - 2, width + 6, width + 4, ST77XX_WHITE);
    this->screenwidth = width;
    this->screenheight = width;
    //display->fill(ST77XX_BLACK);
    int min = screenwidth;
     if (screenheight<screenwidth)
         min = screenheight;
    multiply = min/WD;
    offsetsX = posX;//(screenwidth-(WD*multiply))/2;
    offsetsY = posY;//(screenheight-(WD*multiply))/2;
}

void QRcode_ST7789::screenwhite() {
    display->fill(ST77XX_YELLOW);
}

void QRcode_ST7789::screenupdate() {
    // No hay que hacer nada
}

void QRcode_ST7789::drawPixel(int x, int y, int color) {
    if(color==1) {
        color = ST77XX_BLACK;
    } else {
        color = ST77XX_WHITE;
    }
    display->fillRect(x,y,multiply,multiply,color);
}
