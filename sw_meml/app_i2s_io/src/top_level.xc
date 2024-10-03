// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <platform.h>
#include <xs1.h>
#include <stdio.h>

// UART resources
on tile[0] : port p_uart_rx = PORT_UART_RX;

extern void uart_rx_task();
extern void uart_init_task();

void burn(void) {
    while(1) {

    };
}

int main(void){

    par {
        on tile[0]: {
            uart_init_task();
            uart_rx_task();
        }
        on tile[1]: {
            burn();
        }
    }

    return 0;
}
