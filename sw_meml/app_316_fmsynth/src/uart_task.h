#ifndef __UART_TASK_C__
#define __UART_TASK_C__


extern "C" {

#include <xcore/hwtimer.h>


    void uart_init();
    void uart_rx_task();
    void uart_tx_task(hwtimer_t testsend_timer);
}


#endif
