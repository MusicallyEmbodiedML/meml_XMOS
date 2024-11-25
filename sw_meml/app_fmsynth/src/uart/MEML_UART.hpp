#ifndef __MEML_UART_HPP__
#define __MEML_UART_HPP__

extern "C" {
#include "uart.h"
}
#include <vector>
#include <array>
#include <string>
#include <cstring>

#include "../chans_and_data.h"
#include "UART_Common.hpp"

class MEML_UART {
 public:

    MEML_UART(uart_tx_t *uart_tx);
    void Process(char rx);
    void Reset();
    inline bool IsThereAMessage() { return buffer_available_; }
    bool GetMessage(std::vector<std::string> &message);
    bool ParseAndSend(std::vector<std::string> &buffer);
    void RequestState();
    void SendState();
    void SendUIInfo(te_ui_info idx, const std::vector<std::string> &values);

 protected:

    static constexpr unsigned int kBuffer_size = 64;
    std::array<unsigned char, kBuffer_size> buffer_;
    unsigned int buffer_idx_;
    std::vector< std::string > token_buffer_;
    bool buffer_available_;
    bool button_states_[button_nButtons];
    uart_tx_t *uart_tx_;

    void _PrintBufferState();
    void _Split(char *s, const char *delim);
    // Parsing functions
    bool _ParseJoystick(std::vector<std::string> &buffer);
    bool _ParseButton(std::vector<std::string> &buffer);
    bool _ParseSlider(std::vector<std::string> &buffer);
    void _ParseState(std::vector<std::string> &buffer);
    // TX functions
    void _SendMessage(std::string &payload);
};


#endif
