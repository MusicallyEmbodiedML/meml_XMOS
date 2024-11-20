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


struct UART_Common {

    enum msgType
    {
        joystick='j',
        button='b',
        pulse_period='p',
        slider='s',
        state_request='q',
        state_dump='d'
    };

    static constexpr float kFloatToIntConvScaling = 65536;

    static std::string ConcatMessage(std::vector<std::string> msg) {
        std::string result;

        for (size_t i = 0; i < msg.size(); ++i) {
            result += msg[i];
            if (i != msg.size() - 1) { // Add comma if it's not the last element
                result += ",";
            }
        }
        return result;
    }

    static std::vector<std::string> SplitMessage(const std::string& input) {
        static const char delimiter = ',';
        std::vector<std::string> tokens;
        size_t start = 0;
        size_t end = input.find(delimiter);

        while (end != std::string::npos) {
            tokens.push_back(input.substr(start, end - start));
            start = end + 1; // Move past the delimiter
            end = input.find(delimiter, start);
        }

        // Add the last token
        tokens.push_back(input.substr(start));
        return tokens;
    }

    static std::string FormatMessageWithType(msgType msg_type, const std::string& content) {
        return std::string(1, static_cast<char>(msg_type)) + "," + content;
    }

    static char SplitMessage(const std::string& msg_string, std::string& msg_payload) {
        char c;
        size_t commaPos = msg_string.find(',');
        if (commaPos == std::string::npos || commaPos == 0 || commaPos == msg_string.size() - 1) {
            msg_payload.clear();
            return '\0';
        }
        c = msg_string[0];
        msg_payload = msg_string.substr(commaPos + 1);
        return c;
    }

    static std::string FormatAppState(const ts_app_state& app_state) {
        // Preallocate the vector with a fixed size for all fields + checksum
        std::vector<std::string> values(9);
        uint32_t checksum = 0;

        // Populate the vector with values and calculate the checksum
        values[0] = std::to_string(app_state.n_iterations);
        checksum += app_state.n_iterations;

        values[1] = std::to_string(app_state.last_error);
        checksum += static_cast<uint32_t>(app_state.last_error * kFloatToIntConvScaling); // Cast for consistent checksum computation

        values[2] = std::to_string(app_state.exploration_range);
        checksum += static_cast<uint32_t>(app_state.exploration_range * kFloatToIntConvScaling);

        values[3] = std::to_string(static_cast<int>(app_state.app_id));
        checksum += app_state.app_id;

        values[4] = std::to_string(static_cast<int>(app_state.current_dataset));
        checksum += app_state.current_dataset;

        values[5] = std::to_string(static_cast<int>(app_state.current_model));
        checksum += app_state.current_model;

        values[6] = std::to_string(static_cast<int>(app_state.current_nn_mode));
        checksum += app_state.current_nn_mode;

        values[7] = std::to_string(static_cast<int>(app_state.current_expl_mode));
        checksum += app_state.current_expl_mode;

        // Append the checksum as the last value
        values[8] = std::to_string(checksum);

        // Use ConcatMessage to create the final string
        return ConcatMessage(values);
    }

    static bool ExtractAppState(const std::string& input, ts_app_state& app_state) {
        // Split the input string into fields
        std::vector<std::string> fields = SplitMessage(input);
        return ExtractAppState(fields, app_state);
    }

    static bool ExtractAppState(const std::vector<std::string>& fields, ts_app_state& app_state) {

        if (fields.size() != 9) { // Expect 9 fields: 8 values + checksum
            return false;
        }

        // Manually convert each field into the corresponding data type
        app_state.n_iterations = static_cast<uint32_t>(std::stoul(fields[0]));
        app_state.last_error = static_cast<float>(std::stof(fields[1]));
        app_state.exploration_range = static_cast<float>(std::stof(fields[2]));
        app_state.app_id = static_cast<te_app_id>(std::stoi(fields[3]));
        app_state.current_dataset = static_cast<uint8_t>(std::stoi(fields[4]));
        app_state.current_model = static_cast<uint8_t>(std::stoi(fields[5]));
        app_state.current_nn_mode = static_cast<te_nn_mode>(std::stoi(fields[6]));
        app_state.current_expl_mode = static_cast<te_expl_mode>(std::stoi(fields[7]));

        // Extract the checksum from the last field
        uint32_t provided_checksum = static_cast<uint32_t>(std::stoul(fields[8]));

        // Compute the sum of all numeric fields in the structure
        uint32_t computed_checksum = 0;
        computed_checksum += app_state.n_iterations;
        computed_checksum += static_cast<uint32_t>(app_state.last_error * kFloatToIntConvScaling);
        computed_checksum += static_cast<uint32_t>(app_state.exploration_range * kFloatToIntConvScaling);
        computed_checksum += app_state.app_id;
        computed_checksum += app_state.current_dataset;
        computed_checksum += app_state.current_model;
        computed_checksum += app_state.current_nn_mode;
        computed_checksum += app_state.current_expl_mode;

        // Compare the computed checksum with the provided checksum
        return computed_checksum == provided_checksum;
    }

    static void Test() {
        const ts_app_state ref_app_state {
            1000,
            0.33519f,
            0.21948f,
            app_id_machinelisten,
            2,
            1,
            mode_training,
            expl_mode_pretrain
        };

        std::string serialised(FormatAppState(ref_app_state));
        std::printf("TEST- app_state serialised: %s\n", serialised.c_str());
        ts_app_state test_app_state;
        bool result = ExtractAppState(serialised, test_app_state);
        if (result) {
            std::printf("TEST- checksum passed\n");
        } else {
            std::printf("TEST- checksum FAILED\n");
        }

        auto are_equal_ = [] (const ts_app_state& a, const ts_app_state& b) {
            return std::memcmp(&a, &b, sizeof(ts_app_state)) == 0;
        };

        if (are_equal_(ref_app_state, test_app_state)) {
            std::printf("TEST- passed\n");
        } else {
            std::printf("TEST- FAILED\n");
        }
    }
};

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
