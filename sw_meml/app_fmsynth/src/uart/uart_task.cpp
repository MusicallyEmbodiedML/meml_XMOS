#include "uart_task.hpp"

extern "C" {
#include "uart.h"
#include <xs1.h>
#include <platform.h>
#include <stdio.h>
}
#include <memory>

#include "MEML_UART.hpp"
#include "../chans_and_data.h"

static constexpr unsigned int kBaud_rate_ = 115200;

static hwtimer_t tmr_rx_;
static hwtimer_t tmr_tx_;
static uart_rx_t uart_rx_ctx_;
static uart_tx_t uart_tx_ctx_;

static MEML_UART *meml_uart_ptr_ = nullptr;
static uint8_t meml_uart_mem_[sizeof(MEML_UART)];

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
    tmr_tx_ = hwtimer_alloc();
    if (!tmr_rx_ || !tmr_tx_) {
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

    uart_tx_init(
        &uart_tx_ctx_,
        PORT_LEDS,  //X0D14
        kBaud_rate_,
        8,
        UART_PARITY_NONE,
        2,
        tmr_tx_,
        nullptr,
        0,
        nullptr,
        nullptr
    );
    printf("UART- Initialised UART TX\n");

    meml_uart_ptr_ = new (meml_uart_mem_) MEML_UART(&uart_tx_ctx_);
}

void uart_rx_task()
{
    printf("UART- Listening...\n");
    while (true) {
        char rx = uart_rx(&uart_rx_ctx_);
        // if (rx) {
        //     printf("%c", rx);
        // }
        // Process characters from the UART interface
        if (meml_uart_ptr_) {
            meml_uart_ptr_->Process(rx);

            // ...until a message is received in full
            if (meml_uart_ptr_->IsThereAMessage()) {
                std::vector<std::string> message;
                if (meml_uart_ptr_->GetMessage(message)) {

                    // Parse message (and displatch to interface internally)
                    if (meml_uart_ptr_->ParseAndSend(message)) {
                    }
                }
            }
        }
    }
}

void uart_tx_task(hwtimer_t testsend_timer)
{
    while (true) {
        //hwtimer_delay(testsend_timer, static_cast<unsigned int>(1*1e8));
        //uart_tx(&uart_tx_ctx_, '.');
    }
}
