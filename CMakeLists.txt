cmake_minimum_required(VERSION 3.30)
include(mspm0g350x_base.cmake)

project(MSPM0_FreeRTOS_Template C CXX ASM)

include_directories(
    ./Includes
    ./FreeRTOS/include
    ./FreeRTOS/portable/GCC/ARM_CM0/
    ./BSP
    ./Board
    ./Modules/MPU6050
    ./Modules/Tracking
    ./Modules/PID
    ./Modules/Motor
    ./Modules/Button
)

file(GLOB_RECURSE C_SOURCES "./Sources/*.c")
file(GLOB FREERTOS_C_SOURCES "./FreeRTOS/*.c")
file(GLOB FREERTOS_MEMMANG_C_SOURCES "./FreeRTOS/portable/MemMang/heap_4.c")
file(GLOB_RECURSE FREERTOS_PORTABLE_C_SOURCES "./FreeRTOS/portable/GCC/ARM_CM0/*.c")
file(GLOB_RECURSE BSP_SOURCES "./BSP/*.c")  # 添加BSP源文件
file(GLOB_RECURSE Board_SOURCES "./Board/*.c")  # 添加BSP源文件
file(GLOB_RECURSE MODULES_SOURCES "./Modules/*.c")  # 添加Modules源文件

add_executable(${CMAKE_PROJECT_NAME}.elf
    ${SYSCONFIG_SOURCES}
    ${MSPM0_SDK_SOURCES}
    ${MSPM0_STARTUP}
    ${FREERTOS_C_SOURCES}
    ${FREERTOS_PORTABLE_C_SOURCES}
    ${FREERTOS_MEMMANG_C_SOURCES}
    ${C_SOURCES}
    ${BSP_SOURCES}
    ${Board_SOURCES}
    ${MODULES_SOURCES}
)