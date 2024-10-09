#include "uart_task.hpp"

extern "C" {
#include "uart.h"
#include <xs1.h>
#include <platform.h>
#include <xcore/hwtimer.h>
#include <stdio.h>
}

#include "MEML_UART.hpp"
#include "../chans_and_data.h"

static constexpr unsigned int kBaud_rate_ = 115200;

static hwtimer_t tmr_rx_;
static uart_rx_t uart_rx_ctx_;

static MEML_UART meml_uart_;

HIL_UART_RX_CALLBACK_ATTR
static void uart_rx_error_callback_(
    uart_callback_code_t callback_code, void *app_data)
{
    //printf("UART- uart_rx_error: 0x%x\n", callback_code);
    return;
}


void uart_init()
{
    // Resources for UART
    tmr_rx_ = hwtimer_alloc();
    if (!tmr_rx_) {
        printf("UART- Timer not allocated!\n");
    }

    uart_rx_init(
        &uart_rx_ctx_,
        PORT_UART_RX,  //X0D00,
        kBaud_rate_,
        8,
        UART_PARITY_NONE,
        1,
        tmr_rx_,
        NULL, // No buffer
        0,
        NULL, // No rx complete callback
        uart_rx_error_callback_,
        &uart_rx_ctx_
    );

    printf("UART- Initialised UART RX\n");
}

void uart_rx_task()
{
    printf("UART- Listening...\n");
    while (true) {
        char rx = uart_rx(&uart_rx_ctx_);
        if (rx) {
            printf("%c", rx);
        }
        // Process characters from the UART interface
        meml_uart_.Process(rx);

        // ...until a message is received in full
        if (meml_uart_.IsThereAMessage()) {
            std::vector<std::string> message;
            if (meml_uart_.GetMessage(message)) {
                
                // Parse message (and displatch to interface internally)
                if (meml_uart_.ParseAndSend(message)) {
                }
            }
        }    
    }
}
