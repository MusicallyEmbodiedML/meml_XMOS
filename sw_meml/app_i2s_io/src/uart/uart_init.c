#include "uart_init.h"
#include "uart.h"

#include <xs1.h>
#include <platform.h>
#include <xcore/hwtimer.h>
#include <stdio.h>


uart_rx_t uart_rx_ctx;
static hwtimer_t tmr_rx;


void uart_init_task()
{
    tmr_rx = hwtimer_alloc();
    printf("Timer 0x%00000000x allocated.\n", tmr_rx);
    uart_rx_init(
        &uart_rx_ctx,
        PORT_UART_RX,  //X0D00,
        115200,
        8,
        UART_PARITY_NONE,
        1,
        tmr_rx,
        NULL, // No buffer
        0,
        NULL, // No rx complete callback
        NULL,
        &uart_rx_ctx
        );
}
