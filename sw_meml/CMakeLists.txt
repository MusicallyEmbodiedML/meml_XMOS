cmake_minimum_required(VERSION 3.21)
include($ENV{XMOS_CMAKE_PATH}/xcommon.cmake)
project(sw_meml)

add_subdirectory(app_fmsynth)
add_subdirectory(app_euclidean)
