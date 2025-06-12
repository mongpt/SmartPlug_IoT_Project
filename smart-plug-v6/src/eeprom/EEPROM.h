#ifndef EEPROM_H
#define EEPROM_H

#include <memory>
#include "PicoI2C.h"

#define BLOCK_SIZE 64  // Standard block size (including size + CRC)

class EEPROM {
public:
    explicit EEPROM(PicoI2C *i2c, uint16_t device_address);
    ~EEPROM();

    // Updated methods to write and read with block size, metadata, and CRC
    void writeToMemory(uint16_t memory_address, const uint8_t* data, size_t length);
    void readFromMemory(uint16_t memory_address, uint8_t* data, size_t length);
    void writeFloat(uint16_t memory_address, float value);
    float readFloat(uint16_t memory_address);
    void writeUint64ToEEPROM(uint16_t memory_address, uint64_t value);
    uint64_t readUint64FromEEPROM(uint16_t memory_address);
    SemaphoreHandle_t i2c_mutex;

private:
    PicoI2C *i2c;
    uint16_t device_address;

    // CRC Calculation
    uint16_t crc16(const uint8_t* data, size_t length);

};

#endif // EEPROM_H