
# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.12)

#name project
SET(ProjectName Smart_Plug_Project)

set(PICO_BOARD pico_w)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(${ProjectName})

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()

# Set any variables required for importing libraries
SET(FREERTOS_KERNEL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS-KernelV10.6.2)


include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

########### Core HTTP ###########

set(COREHTTP_PATH "${CMAKE_CURRENT_LIST_DIR}/../lib/coreHTTP" CACHE STRING "Common Lib")
set(COREHTTP_PORT "${CMAKE_CURRENT_LIST_DIR}/../port/coreHTTP/" CACHE STRING "Common Lib")
include(coreHTTP.cmake)

SET(URILIB_PATH "${CMAKE_CURRENT_LIST_DIR}/../lib/uri-library" CACHE STRING "Common Lib")
include(urilib.cmake)

SET(JSON_MAKER_DIR "${CMAKE_CURRENT_LIST_DIR}/../lib/json-maker" CACHE STRING "Common Lib")
include(json-maker.cmake)

SET(TINY_JSON_DIR "${CMAKE_CURRENT_LIST_DIR}/../lib/tiny-json" CACHE STRING "Common Lib")
include(tiny-json.cmake)

set(MD5_PATH "${CMAKE_CURRENT_LIST_DIR}/../lib/md5")
include(md5.cmake)


#set(PICO_BOARD pico_w)

#project(${ProjectName})

# Creates a pico-sdk subdirectory in our project for the libraries
#pico_sdk_init()
add_subdirectory(pico_fota_bootloader)

# Add subdirectories
add_subdirectory(src)

