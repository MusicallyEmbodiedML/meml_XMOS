cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(app_i2s_io)

set(APP_HW_TARGET                   ${CMAKE_CURRENT_LIST_DIR}/XCORE-AI-EXPLORER_meml.xn)

set(APP_DEPENDENT_MODULES           "lib_board_support"
                                    "lib_i2s")

set(APP_COMPILER_FLAGS              -O0
                                    -g
                                    -report
                                    -DBOARD_SUPPORT_BOARD=XK_EVK_XU316
                                    -DXMOS_I2S_MASTER=ON)
set(APP_COMPILER_FLAGS_uart_wrapper.cpp -std=c++14)

set(XMOS_SANDBOX_DIR                ${CMAKE_CURRENT_LIST_DIR}../../../)

XMOS_REGISTER_APP()
