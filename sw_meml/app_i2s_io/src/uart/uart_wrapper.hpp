#ifndef __UART_WRAPPER_HPP__
#define __UART_WRAPPER_HPP__

extern "C" {

#include "uart.h"


/**
 * @brief Receive and break down messages from UART to a downstream
 * dispatcher.
 */
void uart_rx_task();

}



#endif  // __UART_WRAPPER_HPP__
