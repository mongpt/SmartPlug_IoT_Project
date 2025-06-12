#ifndef ESPQRST7789_H
#define ESPQRST7789_H
/* ESP_QRcode. tft version for ST7789
 * include this .h if you have a TFT display
 */

#define TFTDISPLAY
#include "pico/stdlib.h"
#include "qrcodedisplay.h"
#include "st7789.h"


class QRcode_ST7789 : public QRcodeDisplay
{
	private:
		ST7789 *display;
        void drawPixel(int x, int y, int color) override;
	public:
		
		explicit QRcode_ST7789(ST7789 *display);

		void init(int width, int posX, int posY);
		void screenwhite() override;
		void screenupdate() override;
};
#endif