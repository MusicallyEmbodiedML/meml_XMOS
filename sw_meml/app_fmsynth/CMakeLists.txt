cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(app_fmsynth)

set(APP_HW_TARGET                   "XCORE-AI-EXPLORER_meml.xn") # This is the XTC tools supplied XN file for the XK_EVK_XU316

set(APP_DEPENDENT_MODULES           "lib_board_support"
                                    "lib_i2s"
                                    "lib_uart_c"
                                    "lib_mlp"
                                    "lib_synth"
                                    "lib_xassert"
                                    "MusicallyEmbodiedML/lib_common")

set(APP_COMPILER_FLAGS              -O0
                                    -g
                                    -lquadflash
                                    -report
                                    -DBOARD_SUPPORT_BOARD=XK_EVK_XU316
                                    -DXMOS_I2S_MASTER=ON)

set(XMOS_SANDBOX_DIR                ${CMAKE_CURRENT_LIST_DIR}../../../)

XMOS_REGISTER_APP()
