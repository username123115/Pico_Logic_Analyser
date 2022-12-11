# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/Users/danieljin/Desktop/Pico/pico-sdk")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(Logic_Analyser C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(Logic_Analyser Logic_Analyser.c )
pico_generate_pio_header(Logic_Analyser ${CMAKE_CURRENT_LIST_DIR}/data_analyser.pio)


pico_set_program_name(Logic_Analyser "Logic_Analyser")
pico_set_program_version(Logic_Analyser "0.1")

pico_enable_stdio_uart(Logic_Analyser 1)
pico_enable_stdio_usb(Logic_Analyser 1)

# Add the standard library to the build
target_link_libraries(Logic_Analyser pico_stdlib)

# Add any user requested libraries
target_link_libraries(Logic_Analyser
        hardware_dma
        hardware_pio
        hardware_irq
        hardware_gpio
        hardware_pwm
        )

pico_add_extra_outputs(Logic_Analyser)

