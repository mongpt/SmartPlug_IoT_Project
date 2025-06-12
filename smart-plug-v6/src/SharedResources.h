#ifndef RP2040_FREERTOS_IRQ_SHAREDRESOURCES_H
#define RP2040_FREERTOS_IRQ_SHAREDRESOURCES_H

#include <memory>
#include "uart/PicoOsUart.h"
#include "i2c/PicoI2C.h"
#include "../FreeRTOS-KernelV10.6.2/include/event_groups.h"
#include "../FreeRTOS-KernelV10.6.2/include/queue.h"


#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5

class SharedResources {
public:

    SharedResources();

    std::shared_ptr<PicoOsUart> uart420= std::make_shared<PicoOsUart>(UART_NR, UART_TX_PIN, UART_RX_PIN, 9600,  2);
    // Add i2c shared resources here
    std::shared_ptr<PicoI2C> i2cbus = std::make_shared<PicoI2C>(1, 400000);


private:

};


#endif //RP2040_FREERTOS_IRQ_SHAREDRESOURCES_H
