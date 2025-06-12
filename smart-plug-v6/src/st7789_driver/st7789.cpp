#include <cstdio>
#include "ST7789.h"

ST7789::ST7789(const Config& config)
        : config_(config), width_(WIDTH), height_(HEIGHT), gfxFont(nullptr),
          framebuffer_enabled(true), framebuffer_height(HEIGHT/2), framebuffer(nullptr) {
    cursor_x = 0;
    cursor_y = 0;
}

ST7789::~ST7789() {
    freeFramebuffer();
    delete gfxFont;  // If gfxFont was dynamically allocated, delete it
}

void ST7789::allocateFramebuffer(uint16_t width, uint16_t height) {
    freeFramebuffer(); // Ensure any existing buffer is freed
    size_t size = width * height * sizeof(uint16_t);
    framebuffer = (uint16_t*)malloc(size);
    if (framebuffer) {
        memset(framebuffer, 0, size);
        area_width = width;
        area_height = height;
        area_size = width * height;
    } else {
        // Handle allocation failure (e.g., log an error or disable framebuffer)
        framebuffer_enabled = false;
    }
}

void ST7789::freeFramebuffer() {
    if (framebuffer) {
        free(framebuffer);
        framebuffer = nullptr;
    }
}

void ST7789::enableFramebuffer(bool enable) {
    framebuffer_enabled = enable;
    if (!enable) {
        freeFramebuffer();
    }
}

void ST7789::clearScreenBuffer(uint16_t color) {
    if (framebuffer && framebuffer_enabled) {
        for (size_t i = 0; i < area_size; i++) {
            framebuffer[i] = color;
        }
    }
}

// Add this new method to set display window for partial update
void ST7789::setWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    xs_area = x0;
    xe_area = x1 > width_ ? width_ : x1;
    ys_area = y0;
    ye_area = y1 > height_ ? height_ : y1;
    allocateFramebuffer(xe_area - xs_area, ye_area - ys_area); // Allocate for the specific area
    // Set column and row addresses
    caset(xs_area + _xstart, xe_area + _xstart - 1);
    raset(ys_area + _ystart, ye_area + _ystart - 1);
}

// Method to flush the active region of the framebuffer to the screen
void ST7789::flushRegion() {
    if (!framebuffer_enabled || !framebuffer) return;

    ramwr(); // Start memory write
    spi_set_format(config_.spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    data_mode_ = true;
    spi_write16_blocking(config_.spi, framebuffer, area_size); // Send framebuffer to display
    freeFramebuffer(); // Free the buffer immediately after flushing
}

// Replace the existing flush method with this one
void ST7789::flush() {
    // Optionally keep this for full-screen updates, but allocate dynamically if used
    if (!framebuffer_enabled || !framebuffer) return;
    set_cursor(0, 0);
    write(framebuffer, area_size * sizeof(uint16_t));
    freeFramebuffer();
}

/******************************************************************/

void ST7789::command(uint8_t cmd, const uint8_t* data, size_t len) {
    if (config_.gpio_cs > -1) {
        spi_set_format(config_.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    } else {
        spi_set_format(config_.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }
    data_mode_ = false;

    sleep_us(1);
    if (config_.gpio_cs > -1) {
        gpio_put(config_.gpio_cs, 0);
    }
    gpio_put(config_.gpio_dc, 0);
    sleep_us(1);

    spi_write_blocking(config_.spi, &cmd, sizeof(cmd));

    if (len) {
        sleep_us(1);
        gpio_put(config_.gpio_dc, 1);
        sleep_us(1);
        spi_write_blocking(config_.spi, data, len);
    }

    sleep_us(1);
    if (config_.gpio_cs > -1) {
        gpio_put(config_.gpio_cs, 1);
    }
    gpio_put(config_.gpio_dc, 1);
    sleep_us(1);
}

void ST7789::caset(uint16_t xs, uint16_t xe) {
    uint8_t data[] = { static_cast<uint8_t>(xs >> 8), static_cast<uint8_t>(xs & 0xff),
                       static_cast<uint8_t>(xe >> 8), static_cast<uint8_t>(xe & 0xff) };
    command(0x2a, data, sizeof(data));  // CASET (2Ah): Column Address Set
}

void ST7789::raset(uint16_t ys, uint16_t ye) {
    uint8_t data[] = { static_cast<uint8_t>(ys >> 8), static_cast<uint8_t>(ys & 0xff),
                       static_cast<uint8_t>(ye >> 8), static_cast<uint8_t>(ye & 0xff) };
    command(0x2b, data, sizeof(data));  // RASET (2Bh): Row Address Set
}

void ST7789::init() {
    spi_init(config_.spi, 125 * 1000 * 1000);
    if (config_.gpio_cs > -1) {
        spi_set_format(config_.spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    } else {
        spi_set_format(config_.spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    }

    gpio_set_function(config_.gpio_din, GPIO_FUNC_SPI);
    gpio_set_function(config_.gpio_clk, GPIO_FUNC_SPI);

    if (config_.gpio_cs > -1) {
        gpio_init(config_.gpio_cs);
        gpio_set_dir(config_.gpio_cs, GPIO_OUT);
        gpio_put(config_.gpio_cs, 1);
    }

    gpio_init(config_.gpio_dc);
    gpio_init(config_.gpio_rst);
    gpio_init(config_.gpio_bl);

    gpio_set_dir(config_.gpio_dc, GPIO_OUT);
    gpio_set_dir(config_.gpio_rst, GPIO_OUT);
    gpio_set_dir(config_.gpio_bl, GPIO_OUT);

    gpio_put(config_.gpio_dc, 1);
    gpio_put(config_.gpio_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Define variables to hold the command data
    uint8_t colmod_data = 0x55;  // COLMOD (3Ah): Interface Pixel Format
    uint8_t madctl_data = 0x00;  // MADCTL (36h): Memory Data Access Control

    command(0x01, nullptr, 0);  // SWRESET (01h): Software Reset
    vTaskDelay(pdMS_TO_TICKS(150));
    command(0x11, nullptr, 0);  // SLPOUT (11h): Sleep Out
    vTaskDelay(pdMS_TO_TICKS(50));
    //command(0x3a, (uint8_t[]){0x55}, 1);  // COLMOD (3Ah): Interface Pixel Format
    command(0x3A, &colmod_data, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    //command(0x36, (uint8_t[]){0x00}, 1);  // MADCTL (36h): Memory Data Access Control
    command(0x36, &madctl_data, 1);
    caset(0, width_);
    raset(0, height_);
    command(0x21, nullptr, 0);  // INVON (21h): Display Inversion On
    vTaskDelay(pdMS_TO_TICKS(10));
    command(0x13, nullptr, 0);  // NORON (13h): Normal Display Mode On
    vTaskDelay(pdMS_TO_TICKS(10));
    command(0x29, nullptr, 0);  // DISPON (29h): Display On
    vTaskDelay(pdMS_TO_TICKS(10));

    gpio_put(config_.gpio_bl, 1);
}

void ST7789::ramwr() const {
    busy_wait_us(1);
    if (config_.gpio_cs > -1) {
        gpio_put(config_.gpio_cs, 0);
    }
    gpio_put(config_.gpio_dc, 0);
    busy_wait_us(1);

    uint8_t cmd = 0x2c;
    spi_write_blocking(config_.spi, &cmd, sizeof(cmd));

    busy_wait_us(1);
    if (config_.gpio_cs > -1) {
        gpio_put(config_.gpio_cs, 0);
    }
    gpio_put(config_.gpio_dc, 1);
    busy_wait_us(1);
}

void ST7789::write(const void* data, size_t len) {
    if (!data_mode_) {
        ramwr();
        if (config_.gpio_cs > -1) {
            spi_set_format(config_.spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
        } else {
            spi_set_format(config_.spi, 16, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
        }
        data_mode_ = true;
    }

    spi_write16_blocking(config_.spi, static_cast<const uint16_t*>(data), len / 2);
}

void ST7789::put(uint16_t pixel) {
    write(&pixel, sizeof(pixel));
}

void ST7789::fill(uint16_t pixel) {
    int num_pixels = (width_+0) * (height_+0);
    set_cursor(0,0);
    for (int i = 0; i < num_pixels; i++) {
        if (framebuffer_enabled) {
            framebuffer[i] = pixel;  // Store pixel in framebuffer
        } else {
            put(pixel);  // Directly draw to screen
        }
    }
}

void ST7789::set_cursor(uint16_t x, uint16_t y) {
    caset(x+_xstart, width_+_xstart);
    raset(y+_ystart, height_+_ystart);
}

void ST7789::verticalScroll(uint16_t row) {
    uint8_t data[] = { static_cast<uint8_t>((row >> 8) & 0xff), static_cast<uint8_t>(row & 0x00ff) };
    command(0x37, data, sizeof(data));  // VSCSAD (37h): Vertical Scroll Start Address of RAM
}

/*!
    @brief  Set origin of (0,0) and orientation of TFT display
    @param  m  The index for rotation, from 0-3 inclusive
*/
void ST7789::setRotation(uint8_t m) {
    if (width_ == 240 && height_ == 240) {
        // 1.3", 1.54" displays (right justified)
        _rowstart = (320 - height_);
        _rowstart2 = 0;
        _colstart = _colstart2 = (240 - width_);
    } else if (width_ == 135 && height_ == 240) {
        // 1.14" display (centered, with odd size)
        _rowstart = _rowstart2 = (int)((320 - height_) / 2);
        // This is the only device currently supported device that has different
        // values for _colstart & _colstart2. You must ensure that the extra
        // pixel lands in _colstart and not in _colstart2
        _colstart = (int)((240 - width_ + 1) / 2);
        _colstart2 = (int)((240 - width_) / 2);
    } else {
        // 1.47", 1.69, 1.9", 2.0" displays (centered)
        _rowstart = _rowstart2 = (int)((320 - height_) / 2);
        _colstart = _colstart2 = (int)((240 - width_) / 2);
    }

    uint8_t madctl = 0;

    rotation = m & 3; // can't be higher than 3

    switch (rotation) {
        case 0:
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
            _xstart = _colstart;
            _ystart = _rowstart;
            _width = width_;
            _height = height_;
            width_ = _width;
            height_ = _height;
            break;
        case 1:
            madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
            _xstart = _rowstart;
            _ystart = _colstart2;
            _height = width_;
            _width = height_;
            width_ = _width;
            height_ = _height;
            break;
        case 2:
            madctl = ST77XX_MADCTL_RGB;
            _xstart = _colstart2;
            _ystart = _rowstart2;
            _width = width_;
            _height = height_;
            width_ = _width;
            height_ = _height;
            break;
        case 3:
            madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
            _xstart = _rowstart2;
            _ystart = _colstart;
            _height = width_;
            _width = height_;
            width_ = _width;
            height_ = _height;
            break;

    }
    command(ST77XX_MADCTL, &madctl, 1);
}
/**************************************************************************/
//////////////////////////////////////////////////////////////////////
void ST7789::drawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= width_ || y >= height_) return;
    if (framebuffer_enabled && framebuffer) {
        if (x >= xs_area && x < xe_area && y >= ys_area && y < ye_area) {
            framebuffer[(y - ys_area) * area_width + (x - xs_area)] = color;
        }
    } else {
        set_cursor(x, y);
        put(color);
    }
}

void ST7789::writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }
    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }
    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep;
    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }
    for (; x0 <= x1; x0++) {
        if (steep) {
            drawPixel(y0, x0, color);
        } else {
            drawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void ST7789::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    writeLine(x, y, x, y + h - 1, color);
}

void ST7789::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    writeLine(x, y, x + w - 1, y, color);
}

void ST7789::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    // Update in subclasses if desired!
    if (x0 == x1) {
        if (y0 > y1)
            _swap_int16_t(y0, y1);
        drawFastVLine(x0, y0, y1 - y0 + 1, color);
    } else if (y0 == y1) {
        if (x0 > x1)
            _swap_int16_t(x0, x1);
        drawFastHLine(x0, y0, x1 - x0 + 1, color);
    } else {
        writeLine(x0, y0, x1, y1, color);
    }
}

/*!
   @brief    Write a pixel, overwrite in subclasses if startWrite is defined!
    @param   x   x coordinate
    @param   y   y coordinate
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void ST7789::writePixel(int16_t x, int16_t y, uint16_t color) {
    drawPixel(x, y, color);
}
/**************************************************************************/
/*!
   @brief    Write a perfectly vertical line, overwrite in subclasses if
   startWrite is defined!
    @param    x   Top-most x coordinate
    @param    y   Top-most y coordinate
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void ST7789::writeFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color) {
    // Overwrite in subclasses if startWrite is defined!
    // Can be just writeLine(x, y, x, y+h-1, color);
    // or writeFillRect(x, y, 1, h, color);
    drawFastVLine(x, y, h, color);
}

/**************************************************************************/
/*!
   @brief    Write a perfectly horizontal line, overwrite in subclasses if
   startWrite is defined!
    @param    x   Left-most x coordinate
    @param    y   Left-most y coordinate
    @param    w   Width in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void ST7789::writeFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color) {
    // Overwrite in subclasses if startWrite is defined!
    // Example: writeLine(x, y, x+w-1, y, color);
    // or writeFillRect(x, y, w, 1, color);
    drawFastHLine(x, y, w, color);
}

/**************************************************************************/
/*!
   @brief    Fill a rectangle completely with one color. Update in subclasses if
   desired!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void ST7789::fillRect(int16_t x, int16_t y, int16_t w, int16_t h,
                      uint16_t color) {
    for (int16_t i = x; i < x + w; i++) {
        writeFastVLine(i, y, h, color);
    }
}
/**************************************************************************/
/*!
   @brief    Write a rectangle completely with one color, overwrite in
   subclasses if startWrite is defined!
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
   @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void ST7789::writeFillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    // Overwrite in subclasses if desired!
    fillRect(x, y, w, h, color);
}
/**************************************************************************/
/*!
   @brief   Draw a rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void ST7789::drawRect(int16_t x, int16_t y, int16_t w, int16_t h,
                      uint16_t color) {
    writeFastHLine(x, y, w, color);
    writeFastHLine(x, y + h - 1, w, color);
    writeFastVLine(x, y, h, color);
    writeFastVLine(x + w - 1, y, h, color);
}

/**************************************************************************/
/*!
   @brief    Draw a circle outline
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void ST7789::drawCircle(int16_t x0, int16_t y0, int16_t r,
                        uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    writePixel(x0, y0 + r, color);
    writePixel(x0, y0 - r, color);
    writePixel(x0 + r, y0, color);
    writePixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        writePixel(x0 + x, y0 + y, color);
        writePixel(x0 - x, y0 + y, color);
        writePixel(x0 + x, y0 - y, color);
        writePixel(x0 - x, y0 - y, color);
        writePixel(x0 + y, y0 + x, color);
        writePixel(x0 - y, y0 + x, color);
        writePixel(x0 + y, y0 - x, color);
        writePixel(x0 - y, y0 - x, color);
    }
}

/**************************************************************************/
/*!
    @brief    Quarter-circle drawer, used to do circles and roundrects
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    cornername  Mask bit #1 or bit #2 to indicate which quarters of
   the circle we're doing
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void ST7789::drawCircleHelper(int16_t x0, int16_t y0, int16_t r,
                              uint8_t cornername, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        if (cornername & 0x4) {
            writePixel(x0 + x, y0 + y, color);
            writePixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            writePixel(x0 + x, y0 - y, color);
            writePixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            writePixel(x0 - y, y0 + x, color);
            writePixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            writePixel(x0 - y, y0 - x, color);
            writePixel(x0 - x, y0 - y, color);
        }
    }
}

/**************************************************************************/

/*!
    @brief  Quarter-circle drawer with fill, used for circles and roundrects
    @param  x0       Center-point x coordinate
    @param  y0       Center-point y coordinate
    @param  r        Radius of circle
    @param  corners  Mask bits indicating which quarters we're doing
    @param  delta    Offset from center-point, used for round-rects
    @param  color    16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void ST7789::fillCircleHelper(int16_t x0, int16_t y0, int16_t r,
                              uint8_t corners, int16_t delta,
                              uint16_t color) {

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    int16_t px = x;
    int16_t py = y;

    delta++; // Avoid some +1's in the loop

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if (x < (y + 1)) {
            if (corners & 1)
                writeFastVLine(x0 + x, y0 - y, 2 * y + delta, color);
            if (corners & 2)
                writeFastVLine(x0 - x, y0 - y, 2 * y + delta, color);
        }
        if (y != py) {
            if (corners & 1)
                writeFastVLine(x0 + py, y0 - px, 2 * px + delta, color);
            if (corners & 2)
                writeFastVLine(x0 - py, y0 - px, 2 * px + delta, color);
            py = y;
        }
        px = x;
    }
}

/**************************************************************************/
/*!
   @brief    Draw a circle with filled color
    @param    x0   Center-point x coordinate
    @param    y0   Center-point y coordinate
    @param    r   Radius of circle
    @param    color 16-bit 5-6-5 Color to fill with
*/
/**************************************************************************/
void ST7789::fillCircle(int16_t x0, int16_t y0, int16_t r,
                        uint16_t color) {
    writeFastVLine(x0, y0 - r, 2 * r + 1, color);
    fillCircleHelper(x0, y0, r, 3, 0, color);
}

/**************************************************************************/

/*!
   @brief   Draw a rounded rectangle with no fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/

void ST7789::drawRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                           int16_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if (r > max_radius)
        r = max_radius;
    // smarter version
    writeFastHLine(x + r, y, w - 2 * r, color);         // Top
    writeFastHLine(x + r, y + h - 1, w - 2 * r, color); // Bottom
    writeFastVLine(x, y + r, h - 2 * r, color);         // Left
    writeFastVLine(x + w - 1, y + r, h - 2 * r, color); // Right
    // draw four corners
    drawCircleHelper(x + r, y + r, r, 1, color);
    drawCircleHelper(x + w - r - 1, y + r, r, 2, color);
    drawCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
    drawCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

/**************************************************************************/
/*!
   @brief   Draw a rounded rectangle with fill color
    @param    x   Top left corner x coordinate
    @param    y   Top left corner y coordinate
    @param    w   Width in pixels
    @param    h   Height in pixels
    @param    r   Radius of corner rounding
    @param    color 16-bit 5-6-5 Color to draw/fill with
*/
/**************************************************************************/
void ST7789::fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h,
                           int16_t r, uint16_t color) {
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if (r > max_radius)
        r = max_radius;
    // smarter version
    writeFillRect(x + r, y, w - 2 * r, h, color);
    // draw four corners
    fillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    fillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

/**************************************************************************/
/*!
   @brief   Draw a triangle with no fill color
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to draw with
*/
/**************************************************************************/
void ST7789::drawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2, uint16_t color) {
    drawLine(x0, y0, x1, y1, color);
    drawLine(x1, y1, x2, y2, color);
    drawLine(x2, y2, x0, y0, color);
}

/**************************************************************************/
/*!
   @brief     Draw a triangle with color-fill
    @param    x0  Vertex #0 x coordinate
    @param    y0  Vertex #0 y coordinate
    @param    x1  Vertex #1 x coordinate
    @param    y1  Vertex #1 y coordinate
    @param    x2  Vertex #2 x coordinate
    @param    y2  Vertex #2 y coordinate
    @param    color 16-bit 5-6-5 Color to fill/draw with
*/
/**************************************************************************/
void ST7789::fillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2, uint16_t color) {

    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_int16_t(y0, y1);
        _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_int16_t(y2, y1);
        _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_int16_t(y0, y1);
        _swap_int16_t(x0, x1);
    }

    if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if (x1 < a)
            a = x1;
        else if (x1 > b)
            b = x1;
        if (x2 < a)
            a = x2;
        else if (x2 > b)
            b = x2;
        writeFastHLine(a, y0, b - a + 1, color);
        return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0,
            dx12 = x2 - x1, dy12 = y2 - y1;
    int32_t sa = 0, sb = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if (y1 == y2)
        last = y1; // Include y1 scanline
    else
        last = y1 - 1; // Skip it

    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b)
            _swap_int16_t(a, b);
        writeFastHLine(a, y, b - a + 1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if (a > b)
            _swap_int16_t(a, b);
        writeFastHLine(a, y, b - a + 1, color);
    }
}
/**************************************************************************/

void ST7789::drawChar(int16_t x, int16_t y, unsigned char c, uint16_t color, uint16_t bg, uint8_t size_x, uint8_t size_y) {

    if (!gfxFont) { // 'Classic' built-in font
        if ((x >= width_) ||              // Clip right
            (y >= height_) ||             // Clip bottom
            ((x + 6 * size_x - 1) < 0) || // Clip left
            ((y + 8 * size_y - 1) < 0))   // Clip top
            return;

        if (!_cp437 && (c >= 176))
            c++; // Handle 'classic' charset behavior

        for (int8_t i = 0; i < 5; i++) { // Char bitmap = 5 columns
            uint8_t line = pgm_read_byte(&font[c * 5 + i]);
            for (int8_t j = 0; j < 8; j++, line >>= 1) {
                if (line & 1) {
                    if (size_x == 1 && size_y == 1)
                        writePixel(x + i, y + j, color);
                    else
                        writeFillRect(x + i * size_x, y + j * size_y, size_x, size_y,
                                      color);
                } else if (bg != color) {
                    if (size_x == 1 && size_y == 1)
                        writePixel(x + i, y + j, bg);
                    else
                        writeFillRect(x + i * size_x, y + j * size_y, size_x, size_y, bg);
                }
            }
        }
        if (bg != color) { // If opaque, draw vertical line for last column
            if (size_x == 1 && size_y == 1)
                writeFastVLine(x + 5, y, 8, bg);
            else
                writeFillRect(x + 5 * size_x, y, size_x, 8 * size_y, bg);
        }

    } else { // Custom font

        // Character is assumed previously filtered by write() to eliminate
        // newlines, returns, non-printable characters, etc.  Calling
        // drawChar() directly with 'bad' characters of font may cause mayhem!

        c -= (uint8_t)pgm_read_byte(&gfxFont->first);
        GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c);
        uint8_t *bitmap = pgm_read_bitmap_ptr(gfxFont);

        uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
        uint8_t w = pgm_read_byte(&glyph->width), h = pgm_read_byte(&glyph->height);
        int8_t xo = pgm_read_byte(&glyph->xOffset),
                yo = pgm_read_byte(&glyph->yOffset);
        uint8_t xx, yy, bits = 0, bit = 0;
        int16_t xo16 = 0, yo16 = 0;

        if (size_x > 1 || size_y > 1) {
            xo16 = xo;
            yo16 = yo;
        }

        for (yy = 0; yy < h; yy++) {
            for (xx = 0; xx < w; xx++) {
                if (!(bit++ & 7)) {
                    bits = pgm_read_byte(&bitmap[bo++]);
                }
                if (bits & 0x80) {
                    if (size_x == 1 && size_y == 1) {
                        writePixel(x + xo + xx, y + yo + yy, color);
                    } else {
                        writeFillRect(x + (xo16 + xx) * size_x, y + (yo16 + yy) * size_y,
                                      size_x, size_y, color);
                    }
                }
                bits <<= 1;
            }
        }
    } // End classic vs custom font
}

size_t ST7789::write(uint8_t c) {

    if (!gfxFont) { // 'Classic' built-in font
        if (c == '\n') {              // Newline?
            cursor_x = 0;               // Reset x to zero,
            cursor_y += textsize_y * 8; // advance y one line
        } else if (c != '\r') {       // Ignore carriage returns
            if (wrap && ((cursor_x + textsize_x * 6) > width_)) { // Off right?
                cursor_x = 0;                                       // Reset x to zero,
                cursor_y += textsize_y * 8; // advance y one line
            }
            drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x,
                     textsize_y);
            cursor_x += textsize_x * 6; // Advance x one char
        }

    } else { // Custom font
        if (c == '\n') {
            cursor_x = 0;
            cursor_y +=
                    (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
        } else if (c != '\r') {
            uint8_t first = pgm_read_byte(&gfxFont->first);
            if ((c >= first) && (c <= (uint8_t)pgm_read_byte(&gfxFont->last))) {
                GFXglyph *glyph = pgm_read_glyph_ptr(gfxFont, c - first);
                uint8_t w = pgm_read_byte(&glyph->width),
                        h = pgm_read_byte(&glyph->height);
                if ((w > 0) && (h > 0)) { // Is there an associated bitmap?
                    int16_t xo = (int8_t)pgm_read_byte(&glyph->xOffset); // sic
                    if (wrap && ((cursor_x + textsize_x * (xo + w)) > width_)) {
                        cursor_x = 0;
                        cursor_y += (int16_t)textsize_y * (uint8_t)pgm_read_byte(&gfxFont->yAdvance);
                    }
                    drawChar(cursor_x, cursor_y, c, textcolor, textbgcolor, textsize_x, textsize_y);
                }
                cursor_x += (uint8_t)pgm_read_byte(&glyph->xAdvance) * (int16_t)textsize_x;
            }
        }
    }
    return 1;
}

size_t ST7789::write(const uint8_t *buffer, size_t size)
{
    size_t n = 0;
    while (size--) {
        if (write(*buffer++)) n++;
        else break;
    }
    return n;
}

size_t ST7789::write(const char *str) {
    if (str == nullptr) return 0;
    return write((const uint8_t *)str, strlen(str));
}
/**********************************************************************/
/*!
    @brief   Set text font color with transparant background
    @param   c   16-bit 5-6-5 Color to draw text with
    @note    For 'transparent' background, background and foreground
             are set to same color rather than using a separate flag.
  */
/**********************************************************************/
void ST7789::setTextColor(uint16_t c) { textcolor = textbgcolor = c; }

/**********************************************************************/
/*!
  @brief   Set text font color with custom background color
  @param   c   16-bit 5-6-5 Color to draw text with
  @param   bg  16-bit 5-6-5 Color to draw background/fill with
*/
/**********************************************************************/
void ST7789::setTextColor(uint16_t c, uint16_t bg) {
    textcolor = c;
    textbgcolor = bg;
}

/**********************************************************************/
/*!
@brief  Set whether text that is too long for the screen width should
        automatically wrap around to the next line (else clip right).
@param  w  true for wrapping, false for clipping
*/
/**********************************************************************/
void ST7789::setTextWrap(bool w) { wrap = w; }

/**********************************************************************/
/*!
  @brief  Enable (or disable) Code Page 437-compatible charset.
          There was an error in glcdfont.c for the longest time -- one
          character (#176, the 'light shade' block) was missing -- this
          threw off the index of every character that followed it.
          But a TON of code has been written with the erroneous
          character indices. By default, the library uses the original
          'wrong' behavior and old sketches will still work. Pass
          'true' to this function to use correct CP437 character values
          in your code.
  @param  x  true = enable (new behavior), false = disable (old behavior)
*/
/**********************************************************************/
void ST7789::cp437(bool x) { _cp437 = x; }
/**********************************************************************/
/*!
    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel
   that much bigger.
    @param  s_x  Desired text width magnification level in X-axis. 1 is default
    @param  s_y  Desired text width magnification level in Y-axis. 1 is default
*/
/**************************************************************************/
void ST7789::setTextSize(uint8_t s_x, uint8_t s_y) {
    textsize_x = (s_x > 0) ? s_x : 1;
    textsize_y = (s_y > 0) ? s_y : 1;
}
/**************************************************************************/
/*!
    @brief   Set text 'magnification' size. Each increase in s makes 1 pixel
   that much bigger.
    @param  s  Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc
*/
/**************************************************************************/

void ST7789::setTextSize(uint8_t s) { setTextSize(s, s); }
/**************************************************************************/

size_t ST7789::print(const char str[]) {
    return write(str);
}

size_t ST7789::println(void) {
    return write("\r\n");
}

size_t ST7789::println(const char c[]) {
    size_t n = print(c);
    n += println();
    return n;
}

size_t ST7789::printChar(char c)
{
    return write(c);
}

size_t ST7789::print_ulong_base(unsigned long n, uint8_t base)
{
    char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
    char *str = &buf[sizeof(buf) - 1];

    *str = '\0';

    // prevent crash if called with base == 1
    if (base < 2) base = 10;

    do {
        char c = n % base;
        n /= base;

        *--str = c < 10 ? c + '0' : c + 'A' - 10;
    } while(n);

    return write(str);
}

size_t ST7789::print_long_base(long n, int base)
{
    if (base == 0) {
        return write(n);
    } else if (base == 10) {
        if (n < 0) {
            int t = printChar('-');
            n = -n;
            return print_ulong_base(n, 10) + t;
        }
        return print_ulong_base(n, 10);
    } else {
        return print_ulong_base(n, base);
    }
}

size_t ST7789::printFloat(double number, uint8_t digits) {
    size_t n = 0;

    if (std::isnan(number)) return print("nan");
    if (std::isinf(number)) return print("inf");
    if (number > 4294967040.0) return print ("ovf");
    if (number < -4294967040.0) return print ("ovf");

    // Handle negative numbers
    if (number < 0.0) {
        n += printChar('-');
        number = -number;
    }

    // Round correctly so that print(1.999, 2) prints as "2.00"
    double rounding = 0.5;
    for (uint8_t i = 0; i < digits; ++i)
        rounding /= 10.0;

    number += rounding;

    // Extract the integer part of the number and print it
    auto int_part = (unsigned long)number;
    double remainder = number - (double)int_part;
    n += print_ulong_base(int_part, 10);

    // Print the decimal point if there are digits beyond it
    if (digits > 0) {
        n += printChar('.');
    }

    // Extract digits from the remainder one at a time
    while (digits-- > 0) {
        remainder *= 10.0;
        auto toPrint = (unsigned long)(remainder);
        n += print_ulong_base(toPrint, 10);
        remainder -= toPrint;
    }

    return n;
}

/**************************************************************************/
/*!
    @brief Set the font to display when print()ing, either custom or default
    @param  f  The GFXfont object, if NULL use built in 6x8 font
*/
/**************************************************************************/
void ST7789::setFont(const GFXfont *f) {
    if (f) {          // Font struct pointer passed in?
        if (!gfxFont) { // And no current font struct?
            // Switching from classic to new font behavior.
            // Move cursor pos down 6 pixels so it's on baseline.
            cursor_y += 6;
        }
    } else if (gfxFont) { // NULL passed.  Current font struct defined?
        // Switching from new to classic font behavior.
        // Move cursor pos up 6 pixels so it's at top-left of char.
        cursor_y -= 6;
    }
    gfxFont = (GFXfont *)f;
}

/**************************************************************************/
