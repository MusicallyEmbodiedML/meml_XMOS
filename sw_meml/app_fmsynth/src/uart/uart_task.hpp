#ifndef __UART_TASK_CPP__
#define __UART_TASK_CPP__


extern "C" {

#include <xcore/hwtimer.h>


    void uart_init();
    void uart_rx_task();
    void uart_tx_task(hwtimer_t testsend_timer);
}

void uart_update_loss(float loss);


#endif
