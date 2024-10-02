## Source files
file(GLOB_RECURSE LIB_C_SOURCES   RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.c )
file(GLOB_RECURSE LIB_CXX_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cc)
file(GLOB_RECURSE LIB_XC_SOURCES  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.xc)
file(GLOB_RECURSE LIB_ASM_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.S )

## Name
set(LIB_NAME lib_uart_c)
set(LIB_VERSION 1.0.0)
set(LIB_ARCHS xs3a)
## Compiler/linker options
set(LIB_ARCHIVE_INCLUDES api src)
set(LIB_ARCHIVE_C_SRCS ${LIB_C_SOURCES})
set(LIB_ARCHIVE_CXX_SRCS ${LIB_CXX_SOURCES})
set(LIB_ARCHIVE_ASM_SRCS ${LIB_ASM_SOURCES})
set(LIB_ARCHIVE_XC_SRCS ${LIB_XC_SOURCES})
set(LIB_ARCHIVE_COMPILER_FLAGS -O3)
set(LIB_ARCHIVE_DEPENDENT_MODULES "")

XMOS_REGISTER_STATIC_LIB()
