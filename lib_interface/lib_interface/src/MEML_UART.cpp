#include "MEML_UART.hpp"

#include "interface.hpp"
#include "chans_and_data.h"

#include <string>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <cstdio>


MEML_UART::MEML_UART(uart_tx_t *uart_tx) :
        buffer_idx_(0),
        button_states_{false},
        uart_tx_(uart_tx)
{
    // Ensure consistency of nn modes and expl modes
    if (meml_interface) {
        meml_interface->SetToggleButton(toggle_training, gAppState.current_nn_mode);
        meml_interface->SetToggleButton(toggle_explmode, gAppState.current_expl_mode);
    }

    // Send state as soon as woken up
    SendState();
    std::printf("UART- State sent on bootup.\n");
}


void MEML_UART::Process(char rx)
{
    if (rx == '\n') {
        // Terminated line of parameters
        _PrintBufferState();

        // Tokenise and make package
        _Split(reinterpret_cast<char*>(buffer_.data()), ",");

        // Reset
        Reset();
   } else {
        buffer_available_ = 0;
        // Keep populating buffers
        if (buffer_idx_ >= kBuffer_size-1) {
            // Wrap around
            buffer_idx_ = 0;
        }
        buffer_[buffer_idx_] = rx;
        buffer_idx_++;
    }
}


void MEML_UART::Reset()
{
    buffer_idx_ = 0;
}


bool MEML_UART::GetMessage(std::vector<std::string> &message)
{
    if (buffer_available_) {
        // This flag is NOT thread-safe - upgrade to atomic and
        // std::binary_semaphore if required

        for (auto t : token_buffer_) {
            message.push_back(t);
        }
    }
    return buffer_available_;
}


void MEML_UART::_PrintBufferState()
{
    // Terminate buffer with \0!!!
    if (buffer_idx_ < kBuffer_size-1) {
        buffer_[buffer_idx_] = '\0';
    } else {
        buffer_[buffer_idx_-1] = '\0';
    }
    //std::printf("UART- Buffer: %s\n", buffer_);
}


void MEML_UART::_Split(char *s, const char *delim)
{
    token_buffer_.clear();

    char *pch = std::strtok(s, delim);
    unsigned int pch_counter = 0;

    while (pch != nullptr)
    {
        token_buffer_.push_back(std::string(pch));
        pch = std::strtok(nullptr, delim);
        pch_counter++;
    }

    // If something is found, signal buffer available
    if (pch_counter > 0) {
        buffer_available_ = true;
    }
}

bool MEML_UART::_ParseJoystick(std::vector<std::string> &buffer)
{
    static constexpr float u16_float_scaling = 1.f/65535.f;

    if (buffer.size() != 2) {
        std::printf("UART- Wrong buffer for joystick parse, size=%d!\n", buffer.size());
        return false;
    }

    unsigned int pot_index = std::atoi(buffer[0].c_str());
    if (pot_index >= joystick_nPots) {
        std::printf("UART- Wrong joystick index %s!\n", buffer[0].c_str());
        return false;
    }

    num_t pot_value = std::atof(buffer[1].c_str()) * u16_float_scaling;
    if (pot_value > 1.00001f || pot_value < -0.00001f) {
        std::printf("UART- Wrong joystick value %s!\n", buffer[1].c_str());
        return false;
    }
    meml_interface->SetPot(static_cast<te_joystick_pot>(pot_index), pot_value);

    return true;
}

bool MEML_UART::_ParseButton(std::vector<std::string> &buffer)
{
    if (buffer.size() != 2) {
        std::printf("UART- Wrong buffer for button parse!\n");
        return false;
    }

    unsigned int btn_index = std::atoi(buffer[0].c_str());
    if (btn_index >= button_nButtons) {
        std::printf("UART- Wrong button index %s!\n", buffer[0].c_str());
        return false;
    }

    int8_t btn_value = static_cast<int8_t>(std::atoi(buffer[1].c_str()));

    bool btn_value_bool = false;
    if (btn_index != toggle_explmode &&
        btn_index != toggle_dataset &&
        btn_index != toggle_model) {
        if (btn_value != 0 && btn_value != 1) {
            std::printf("UART- Wrong button value %s!\n", buffer[1].c_str());
            return false;
        }
        btn_value_bool = !static_cast<bool>(btn_value);
    }

    switch (btn_index) {
        case toggle_training:
        case toggle_savedata:
        {
            // Toggles
            meml_interface->SetToggleButton(static_cast<te_button_idx>(btn_index), btn_value_bool);
        } break;
        case button_randomise:
        case button_cleardata:
        case button_clearmodel:
        {
            // Buttons (on press)
            if (btn_value_bool && !button_states_[btn_index]) {
                // Pressed
                meml_interface->SetToggleButton(static_cast<te_button_idx>(btn_index), btn_value_bool);
            }
        } break;
        case toggle_explmode:
        case toggle_dataset:
        case toggle_model: {
            // Dropdowns
            meml_interface->SetToggleButton(static_cast<te_button_idx>(btn_index), btn_value);
        } break;
        case toggle_discretise:
        case toggle_complex:
        default: {
            std::printf("Button not supported in fmsynth.\n");
        }
    } // switch
    button_states_[btn_index] = btn_value_bool;

    return true;
}

bool MEML_UART::_ParseSlider(std::vector<std::string> &buffer)
{
    if (buffer.size() != 2) {
        std::printf("UART- Wrong buffer for slider parse!\n");
        return false;
    }

    unsigned int slider_index = std::atoi(buffer[0].c_str());
    if (slider_index >= slider_nSliders) {
        std::printf("UART- Wrong slider index %s!\n", buffer[0].c_str());
        return false;
    }

    float slider_value = std::atof(buffer[1].c_str());
    if (std::isinf(slider_value) || std::isnan(slider_value)) {
        std::printf("UART- Wrong slider value %s!\n", buffer[1].c_str());
        return false;
    }

    //std::printf("UART- Slider %d: %f\n", slider_index, slider_value);

    meml_interface->SetSlider(
        static_cast<te_slider_idx>(slider_index),
        slider_value
    );

    return true;
}

void MEML_UART::_ParseState(std::vector<std::string> &buffer)
{
    ts_app_state new_state;
    if (!UART_Common::ExtractAppState(buffer, new_state)) {
        std::printf("UART - state corrupted!");
        RequestState();
    }

    gAppState = new_state;
}

bool MEML_UART::_ParseMIDINote(std::vector<std::string> &buffer)
{
    static constexpr float kVelocity_scaling = 1./128.;

    bool ret = true;

    if (buffer.size() < 2) {
        std::printf("UART- malformed MIDI note.");
        return false;
    }

    size_t note_number = std::stoi(buffer[0]);
    float note_velocity = std::stof(buffer[1]);
    note_velocity *= kVelocity_scaling;
    if (note_velocity <= kVelocity_scaling) {
        note_velocity = 0;
    }

    ts_midi_note midi_note {
        note_number,
        note_velocity
    };

    meml_interface->SendMIDI(midi_note);

    return ret;
}

bool MEML_UART::_ParsePulse(std::vector<std::string> &buffer)
{
    if (buffer.size() != 2) {
        std::printf("UART- Wrong buffer for pulse parse!\n");
        return false;
    }

    std::string pulse_string = buffer[1];
    int pulse_int = std::atoi(pulse_string.c_str());
    //std::printf("UART- Pulse %d (%s) microseconds\n", pulse_int, pulse_string.c_str());
    meml_interface->SetPulse(pulse_int);

    return true;
}

bool MEML_UART::ParseAndSend(std::vector<std::string> &buffer)
{
    if (!buffer.size()) {
        std::printf("UART- buffer empty\n");
        return false;
    }
    std::string first_token = buffer[0];
    if (buffer.size() < 2 && first_token.back() != UART_Common::state_request) {
        //std::printf("UART- no payload for token %s\n", first_token.c_str());
        return false;
    }
    std::vector<std::string> payload(buffer.begin()+1, buffer.end());

    const char switch_token = first_token.back();
    switch (switch_token) {
        case UART_Common::joystick: {
            _ParseJoystick(payload);
        } break;
        case UART_Common::button: {
            _ParseButton(payload);
        } break;
        case UART_Common::slider: {
            _ParseSlider(payload);
        } break;
        case UART_Common::state_request: {
            SendState();
        } break;
        case UART_Common::state_dump: {
            _ParseState(payload);
        } break;
        case UART_Common::midi_note: {
            _ParseMIDINote(payload);
        } break;
        case UART_Common::pulse_period: {
            _ParsePulse(payload);
        } break;
        default: {
            //std::printf("UART- message type %c unknown", switch_token);
            return false;
        }
    }

    return true;
}

// UART Transmission methods

void MEML_UART::RequestState()
{
    std::string req_msg(UART_Common::FormatMessageWithType(
        UART_Common::state_request,
        ""
    ));

    _SendMessage(req_msg);
}

void MEML_UART::SendState()
{
    std::string serialised_app_state(
        UART_Common::FormatAppState(gAppState));
    std::string app_state_msg(UART_Common::FormatMessageWithType(
        UART_Common::state_dump,
        serialised_app_state
    ));

    std::printf("UART- Send state: %s\n", app_state_msg.c_str());
    _SendMessage(app_state_msg);
}

void MEML_UART::_SendMessage(std::string &payload)
{
    for (auto &c: payload) {
        uart_tx(uart_tx_, c);
        std::printf("%c", c);
    }
    uart_tx(uart_tx_, '\n');
    std::printf("\n");
}

void MEML_UART::SendUIInfo(te_ui_info idx, const std::vector<std::string> &values)
{
    // Append UI info idx
    std::vector<std::string> values_with_idx{std::to_string(idx)};
    values_with_idx.insert(values_with_idx.end(), values.begin(), values.end());
    // Append message type and send
    std::string values_string(UART_Common::ConcatMessage(values_with_idx));
    std::string payload = UART_Common::FormatMessageWithType(UART_Common::msgType::ui_info, values_string);
    _SendMessage(payload);
}

