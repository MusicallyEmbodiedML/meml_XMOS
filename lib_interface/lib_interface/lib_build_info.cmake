set(LIB_NAME lib_interface)
set(LIB_VERSION 0.0.0)
set(LIB_INCLUDES api)
set(LIB_COMPILER_FLAGS -O0 -g)
set(LIB_DEPENDENT_MODULES "lib_uart_c" "lib_mlp" "lib_common")

XMOS_REGISTER_MODULE()
