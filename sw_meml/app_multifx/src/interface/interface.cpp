#include "interface.hpp"

extern "C" {
   #include <xcore/channel.h>
}

// STL includes
#include <string>
#include <utility>
#include <cstdio>
#include <cstdlib>
#include <cmath>

// Internal C++ includes
#include "../chans_and_data.h"
#include "../mlp/mlp_task.hpp"

#include "matrixMix.hpp"

///
// C++ HELPER CLASSES
///

MEMLInterface::MEMLInterface(chanend_t interface_fmsynth) :
        mode_(mode_inference),
        joystick_current_({ { 0.5, 0.5, 0.5 } }),
        interface_fmsynth_(interface_fmsynth)
{
}

void MEMLInterface::SetPot(te_joystick_pot pot_n, num_t value)
{
    // Update state of joystick
    if (value < 0) {
        value = 0;
    } else if (value > 1.0) {
        value = 1.0;
    }
    joystick_current_.as_array[pot_n] = value;

    // If inference, send down to channel
    if (mode_ == mode_inference) {
        mlp_inference_nochannel(joystick_current_.as_struct);
    }
}

void MEMLInterface::SetToggleButton(te_button_idx button_n, bool state)
{
    switch(button_n) {

        case toggle_training: {
            if (state == mode_inference && mode_ == mode_training) {
                Dataset::Train();
            }
            mode_ = static_cast<te_nn_mode>(state);
            std::string dbg_mode(( mode_ == mode_training ) ? "training" : "inference");
            std::printf("INTF- Mode: %s\n", dbg_mode.c_str());

        } break;

        case button_randomise: {
            if (mode_ == mode_training) {
            // Generate random params
            std::vector<float> rand_params(kN_synthparams);
            MaxtrixMixApp::GenParams(rand_params);

            // Send them down to fmsynth
            chan_out_buf_byte(
                interface_fmsynth_,
                reinterpret_cast<uint8_t *>(rand_params.data()),
                sizeof(num_t) * kN_synthparams
            );

            // Also save them in an intermediate space
            current_fmsynth_params_ = std::move(rand_params);
            std::printf("INTF- Random params\n");
            }
        } break;

        case button_savedata: {
            if (mode_ == mode_training) {
                if (current_fmsynth_params_.size() > 0) {
                    // Save data point
                    std::vector<num_t> input{
                        joystick_current_.as_struct.potX,
                        joystick_current_.as_struct.potY,
                        joystick_current_.as_struct.potRotate,
                        1.f  // bias
                    };
                    Dataset::Add(
                        input, current_fmsynth_params_
                    );
                    std::printf("INTF- Saved data point\n");
                } else {
                    std::printf("INTF- Data point skipped\n");
                }
            }
        } break;

        default: {}
    }
}

///
// C WRAPPER TASK
///

static char meml_interface_mem_[sizeof(MEMLInterface)];
MEMLInterface *meml_interface = nullptr;


void interface_init(chanend_t interface_fmsynth)
{
    meml_interface = new (meml_interface_mem_) MEMLInterface(
    interface_fmsynth
    );
}
