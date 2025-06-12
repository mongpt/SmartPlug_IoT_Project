
include(OTA/files.cmake)
file(GLOB OTAFIELS "${CMAKE_CURRENT_LIST_DIR}/OTA/*.cpp")
file(GLOB EEPROM "${CMAKE_CURRENT_LIST_DIR}/eeprom/*.cpp")
file(GLOB I2C "${CMAKE_CURRENT_LIST_DIR}/i2c/*.cpp")
file(GLOB IPSTACK "${CMAKE_CURRENT_LIST_DIR}/ipstack/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/ipstack/*.c")
file(GLOB MQTT "${CMAKE_CURRENT_LIST_DIR}/MQTT/*.cpp")
file(GLOB NETWORK "${CMAKE_CURRENT_LIST_DIR}/Network/*.cpp")
file(GLOB RTC "${CMAKE_CURRENT_LIST_DIR}/RTC/*.cpp")
file(GLOB ST7789 "${CMAKE_CURRENT_LIST_DIR}/st7789_driver/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/st7789_driver/*.c")
file(GLOB TASKS "${CMAKE_CURRENT_LIST_DIR}/tasks/*.cpp")
#file(GLOB UART "${CMAKE_CURRENT_LIST_DIR}/uart/*.cpp")
file(GLOB SOURCE "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB QRCODE "${CMAKE_CURRENT_LIST_DIR}/QRcodeGen/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/QRcodeGen/*.c")

add_executable(${ProjectName}
        ${OTAFIELS}
        ${EEPROM}
        ${I2C}
        ${IPSTACK}
        ${MQTT}
        ${NETWORK}
        ${RTC}
        ${ST7789}
        ${TASKS}
        #${UART}
        ${SOURCE}
        ${QRCODE}
        main.cpp
        tasks/gpio_output_task.cpp
        tasks/gpio_output_task.h
        tasks/gpio_input_task.cpp
        tasks/gpio_input_task.h
        tasks/internet_task.cpp
        tasks/internet_task.h
        tasks/eeprom_task.cpp
        tasks/eeprom_task.h
        tasks/buzzer_task.cpp
        tasks/buzzer_task.h
        tasks/rtc_task.cpp
        tasks/rtc_task.h
        tasks/common.h
        tasks/common.cpp
        tasks/heap_task.cpp
        tasks/heap_task.h
        tasks/ota_task.cpp
        tasks/ota_task.h

)

# Ensure exceptions are enabled for C++
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

# OR ensure per target (best method)
target_compile_options(${ProjectName} PRIVATE -fexceptions)

set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/QRcodeGen/qrcodegen.cpp PROPERTIES COMPILE_FLAGS -fexceptions)

target_include_directories(${ProjectName} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        #uart
        i2c
        ipstack
        eeprom
        MQTT
        Network
        RTC
        st7789_driver
        tasks
        QRcodeGen
        OTA
        ${PICO_LWIP_CONTRIB_PATH}/apps/httpd
        ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts

)

target_compile_definitions(${ProjectName} PRIVATE
        WIFI_SSID=\"$ENV{WIFI_SSID}\"
        WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
        NO_SYS=0            # don't want NO_SYS (generally this would be in your lwipopts.h)
        #PICO_CYW43_ARCH_DEFAULT_COUNTRY_CODE=CYW43_COUNTRY_FINLAND
)

target_compile_features(${ProjectName} PRIVATE c_std_17 cxx_std_17)

target_link_libraries(${ProjectName}
        pico_stdlib
        hardware_i2c
        hardware_adc
        hardware_spi
        hardware_pwm
        hardware_rtc
        FreeRTOS-Kernel-Heap4
        pico_cyw43_arch_lwip_sys_freertos
        pico_lwip_mbedtls
        pico_mbedtls
        pico_lwip_http
        pico_freertos_httpd_content
        pico_lwip_mqtt
        #pico_cyw43_arch_lwip_threadsafe_background
        # LWIP_PORT
        coreHTTP
        urilib
        json_maker
        tiny_json
        md5
        pico_fota_bootloader_lib
)

# Ignore warnings from lwip code
set_source_files_properties(
        ${PICO_LWIP_PATH}/src/apps/altcp_tls/altcp_tls_mbedtls.c
        PROPERTIES
        COMPILE_OPTIONS "-Wno-unused-result"
)

pico_add_extra_outputs(${ProjectName})
pico_add_library(pico_freertos_httpd_content NOFLAG)


# Disable usb output, enable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

target_compile_definitions(${PROJECT_NAME} PRIVATE
        RELEASE="Smart-plug-v9"
        #OTA_URL="http://172.20.10.12:5000/smart"
        OTA_URL="http://192.168.140.194:5000/smart"

)

# Add pico_fota_bootloader
target_link_options(${ProjectName} PRIVATE
        "-L${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common"
)

pico_set_linker_script(${ProjectName}
        ${CMAKE_CURRENT_SOURCE_DIR}/../pico_fota_bootloader/linker_common/application.ld
)

