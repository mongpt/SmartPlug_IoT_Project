#ifndef ST7789_H
#define ST7789_H

#include "FreeRTOS.h"
#include <task.h>
#include "hardware/spi.h"
#include <cstdint>
#include <cstddef>
#include <cstdlib>
#include <cmath>
#include "hardware/gpio.h"
#include "pico/time.h"
#include <cstring>
#include "gfxfont.h"
#include "glcdfont.c"
#include <algorithm>
#include <memory>

#ifndef pgm_read_byte
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif
#ifndef pgm_read_word
#define pgm_read_word(addr) (*(const unsigned short *)(addr))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(addr) (*(const unsigned long *)(addr))
#endif

#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_BLUE 0x001F
#define ST77XX_CYAN 0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_ORANGE 0xFC00
#define ST77XX_LIGHTGREY  0xC618  // Light Grey  (R=24, G=49, B=24)
#define ST77XX_GREY       0x7BEF  // Neutral Grey (R=15, G=31, B=15)
#define ST77XX_DARKGREY   0x39E7  // Dark Grey   (R=7,  G=14, B=7)
#define ST77XX_BROWN 0x8A22  // Brown (R=17, G=10, B=2)
#define ST77XX_DARKORANGE 0x8200  // Dark Orange (R=16, G=4, B=0)
#define ST77XX_SPICE 0xA300  // Spice (R=20, G=12, B=0)
#define ST77XX_COFFEE 0x6B40  // Coffee (R=13, G=22, B=0)


#define ST77XX_MADCTL_MX 0x40
#define ST77XX_MADCTL_MY 0x80
#define ST77XX_MADCTL_MV 0x20
#define ST77XX_MADCTL_RGB 0x00
#define ST77XX_MADCTL 0x36
#define DEFAULT_TEXT_W 6
#define DEFAULT_TEXT_H 8

static constexpr uint16_t WIDTH = 240;
static constexpr uint16_t HEIGHT = 240;
//static uint16_t framebuffer[(WIDTH * HEIGHT) / 4];

class ST7789 {
public:
    struct Config {
        spi_inst_t* spi;
        uint gpio_din;
        uint gpio_clk;
        int gpio_cs;
        uint gpio_dc;
        uint gpio_rst;
        uint gpio_bl;
    };
    uint16_t cursor_x;     ///< x location to start print()ing text
    uint16_t cursor_y;     ///< y location to start print()ing text
    Config config_;
    uint16_t width_;
    uint16_t height_;
    // for partial updates
    uint16_t area_size;  // Size of the framebuffer of specific area
    uint16_t area_width; // Width of the area needed to be updated
    uint16_t area_height; // Height of the area needed to be updated
    uint16_t xs_area; // Starting x position of the area needed to be updated
    uint16_t xe_area; // Ending x position of the area needed to be updated
    uint16_t ys_area; // Starting y position of the area needed to be updated
    uint16_t ye_area; // Ending y position of the area needed to be updated

    explicit ST7789(const Config& config);

    void command(uint8_t cmd, const uint8_t* data, size_t len);
    void drawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void drawCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
    void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
    void drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void fill(uint16_t pixel);
    void fillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
    void fillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
    void fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
    void init();
    size_t print(const char str[]);
    size_t printFloat(double number, uint8_t digits);
    size_t println();
    size_t println(const char c[]);
    size_t printChar(char c);
    void put(uint16_t pixel);
    void setFont(const GFXfont *f);
    void setRotation(uint8_t m);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t c, uint16_t bg);
    void setTextSize(uint8_t s);
    void setTextSize(uint8_t s_x, uint8_t s_y);
    void setTextWrap(bool w);
    void set_cursor(uint16_t x, uint16_t y);
    void verticalScroll(uint16_t row);
    void write(const void* data, size_t len);
    void writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
    void writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
    void writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    ~ST7789();
    void enableFramebuffer(bool enable);
    void drawPixel(uint16_t x, uint16_t y, uint16_t color);
    void clearScreenBuffer(uint16_t color = 0x0000);
    void flush();
    // Method to set display window for partial update
    void setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
    void flushRegion();

private:
    bool data_mode_ = false;
    uint8_t _colstart;       ///< Some displays need this changed to offset
    uint8_t _rowstart;       ///< Some displays need this changed to offset
    uint8_t _colstart2;     ///< Offset from the right
    uint8_t _rowstart2;     ///< Offset from the bottom
    int16_t _xstart;       ///< Internal framebuffer X offset
    int16_t _ystart;       ///< Internal framebuffer Y offset
    uint16_t _width;       ///< Display width as modified by current rotation
    uint16_t _height;      ///< Display height as modified by current rotation
    uint16_t textcolor;   ///< 16-bit background color for print()
    uint16_t textbgcolor; ///< 16-bit text color for print()
    uint8_t textsize_x;   ///< Desired magnification in X-axis of text to print()
    uint8_t textsize_y;   ///< Desired magnification in Y-axis of text to print()
    uint8_t rotation;     ///< Display rotation (0 thru 3)
    bool wrap;            ///< If set, 'wrap' text at right edge of display
    bool _cp437;          ///< If set, use correct CP437 charset (default is off)
    GFXfont *gfxFont;     ///< Pointer to special font
    uint16_t* framebuffer; // Dynamic framebuffer pointer
    bool framebuffer_enabled;
    uint16_t framebuffer_height; // The actual height of the framebuffer (half of display)


#ifndef _swap_int16_t
#define _swap_int16_t(a, b)       \
  do {                            \
    int16_t t = (a);              \
    (a) = (b);                    \
    (b) = t;                      \
  } while (0)
#endif

    inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfont, uint8_t c) {
        return gfont->glyph + c;
    }

    inline uint8_t *pgm_read_bitmap_ptr(const GFXfont *gfont) {
        return gfont->bitmap;
    }

    void caset(uint16_t xs, uint16_t xe);
    void cp437(bool x = true);
    void drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y);
    size_t print_long_base(long n, int base);
    size_t print_ulong_base(unsigned long n, uint8_t base);
    void ramwr() const;
    void raset(uint16_t ys, uint16_t ye);
    size_t write(uint8_t c);
    size_t write(const uint8_t *buffer, size_t size);
    size_t write(const char *str);
    void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void writePixel(int16_t x, int16_t y, uint16_t color);
    // newly added for dynamic framebuffer
    void allocateFramebuffer(uint16_t width, uint16_t height);
    void freeFramebuffer();
};

#endif // ST7789_H
