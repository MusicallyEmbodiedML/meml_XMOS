#include "interface.hpp"

extern "C" {
   #include <xcore/channel.h>
}

// STL includes
#include <string>
#include <utility>
#include <cstdio>

// Internal C++ includes
#include "../chans_and_data.h"
#include "../mlp/mlp_task.hpp"

#define INTERFACE_STANDALONE 1

#if !(INTERFACE_STANDALONE)
#include "fmsynth_wrapper.hpp"
#include "FMSynth.hpp"
#endif

///
// C++ HELPER CLASSES
///

MEMLInterface::MEMLInterface(chanend_t interface_nn_joystick,
                             chanend_t interface_fmsynth) :
      mode_(mode_inference),
      joystick_current_({ { 0.5, 0.5, 0.5 } }),
      interface_nn_joystick_(interface_nn_joystick),
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
      //std::printf("INTF- Sending joystick state...\n");
#if !(INTERFACE_STANDALONE)
      chan_out_buf_byte(
         interface_nn_joystick_,
         reinterpret_cast<unsigned char *>(joystick_current_.as_array),
         sizeof(ts_joystick_read)
      );
#endif
      //std::printf("INTF- Sent joystick state.\n");
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
            DebugDumpJSON();
         #endif
         }
         mode_ = static_cast<te_nn_mode>(state);
         std::string dbg_mode(( mode_ == mode_training ) ? "training" : "inference");
         std::printf("INTF- Mode: %s\n", dbg_mode.c_str());

      } break;
      case button_randomise: {

         if (mode_ == mode_training) {
            // Generate random params
#if !(INTERFACE_STANDALONE)
            std::vector<float> rand_params(kN_synthparams);
            FMSynth::GenParams(rand_params);

            // Send them down to fmsynth
            chan_out_buf_byte(
               interface_fmsynth_,
               reinterpret_cast<uint8_t *>(rand_params.data()),
               sizeof(num_t) * kN_synthparams
            );

            // Also save them in an intermediate space
            current_fmsynth_params_ = std::move(rand_params);
#endif
            std::printf("INTF- Random params\n");
         }

      } break;
      case button_savedata: {
         
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
      } break;
      default: {}
   }
}

///
// C WRAPPER TASK
///

static char meml_interface_mem_[sizeof(MEMLInterface)];
MEMLInterface *meml_interface = nullptr;


void interface_init(chanend_t interface_nn_joystick,
                    chanend_t interface_fmsynth,
                    chanend_t interface_nn_data,
                    chanend_t interface_nn_train)
{
   meml_interface = new (meml_interface_mem_) MEMLInterface(
   interface_nn_joystick,
   interface_fmsynth
   );
}
