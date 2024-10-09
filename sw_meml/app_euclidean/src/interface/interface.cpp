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
#include "../audio/audio_app.h"

///
// C++ HELPER CLASSES
///

MEMLInterface::MEMLInterface(chanend_t interface_fmsynth,
                             chanend_t interface_pulse,
                             size_t joystick_gridsize) :
      mode_(mode_inference),
      joystick_current_({ { 0.5, 0.5, 0.5 } }),
      interface_fmsynth_(interface_fmsynth),
      interface_pulse_(interface_pulse),
      grid_size_(joystick_gridsize)
{
    if (joystick_gridsize) {
        // Linspace on grid_linspace_
        float spacing = 1.f / joystick_gridsize;
        grid_linspace_.reserve(grid_size_ + 1);
        std::printf("INTF- Discretisation linspace: [");
        for (float n = 0; n <= 1.; n += spacing) {
            grid_linspace_.push_back(n);
            std::printf("%f, ", n);
        }
        std::printf("], grid_size=%d", grid_size_);
    }
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

    if (grid_size_) {
        // Discretise values
        Discretise_(joystick_current_.as_struct);
    }

   // If inference, send down to channel
   if (mode_ == mode_inference) {
      mlp_inference_nochannel(joystick_current_.as_struct);
   }
}

void MEMLInterface::SetPulse(int32_t pulse)
{
    chan_out_word(interface_pulse_, pulse);
}

static void GenParams_(std::vector<float> &param_vector, size_t how_many)
{
    static constexpr float rand_scale = 1.f / static_cast<float>(RAND_MAX);
    for(size_t i=0; i < how_many; i++) {
        param_vector[i] = std::rand() * rand_scale;
    }
}


void MEMLInterface::SetToggleButton(te_button_idx button_n, bool state)
{
   switch(button_n) {
      case toggle_training: {

         if (state == mode_inference && mode_ == mode_training) {
            Dataset::Train();
         #if !(INTERFACE_STANDALONE)
            // Print debug model
            //DebugDumpJSON();
         #endif
         }
         mode_ = static_cast<te_nn_mode>(state);
         std::string dbg_mode(( mode_ == mode_training ) ? "training" : "inference");
         std::printf("INTF- Mode: %s\n", dbg_mode.c_str());

      } break;
      case button_randomise: {

         if (mode_ == mode_training) {
            // Generate random params
            std::vector<float> rand_params(kN_gen_params);
            GenParams_(rand_params, kN_gen_params);

            // Send them down to fmsynth
            chan_out_buf_byte(
               interface_fmsynth_,
               reinterpret_cast<uint8_t *>(rand_params.data()),
               sizeof(num_t) * kN_gen_params
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


float MEMLInterface::DiscretiseOne_(float n)
{
    int idx = static_cast<int>(std::round(n * grid_size_));
    while (idx < 0) {
        idx++;
    }
    while (idx >= grid_size_) {
        idx--;
    }
    return (grid_linspace_[idx]);
}


void MEMLInterface::Discretise_(ts_joystick_read &params)
{
    params.potX = DiscretiseOne_(params.potX);
    params.potY = DiscretiseOne_(params.potY);
    params.potRotate = DiscretiseOne_(params.potRotate);
}


///
// C WRAPPER TASK
///

static char meml_interface_mem_[sizeof(MEMLInterface)];
MEMLInterface *meml_interface = nullptr;


void interface_init(chanend_t interface_fmsynth,
    chanend_t interface_pulse)
{
    meml_interface = new (meml_interface_mem_) MEMLInterface(
        interface_fmsynth,
        interface_pulse,
        0
    );
}
