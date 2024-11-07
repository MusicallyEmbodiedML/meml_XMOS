#ifndef __MEML_INTERFACE_HPP__
#define __MEML_INTERFACE_HPP__

extern "C" {
#include <xcore/chanend.h>
}


#include "../chans_and_data.h"
#include "Data.h"
#include <cstdint>
#include <vector>

#define MEML_IF_CALLBACK_ATTR __attribute__((fptrgroup("meml_interface_callback")))

class MEMLInterface {
 public:

    using GenParamsFn_ptr_t = void (*)(std::vector<float>&);

    MEMLInterface(chanend_t interface_fmsynth,
                  MEML_IF_CALLBACK_ATTR GenParamsFn_ptr_t gen_params_fn_ptr,
                  size_t nn_output_size);
    void SetPot(te_joystick_pot pot_n, num_t value);
    void SetToggleButton(te_button_idx button_n, bool state);

 protected:

    // States
    te_nn_mode mode_;
    union {
        ts_joystick_read as_struct;
        num_t as_array[sizeof(ts_joystick_read)/sizeof(num_t)];
    } joystick_current_;
    std::vector<float> current_fmsynth_params_;
    // Channels for outside comms
    //chanend_t interface_nn_joystick_;
    chanend_t interface_fmsynth_;

    MEML_IF_CALLBACK_ATTR GenParamsFn_ptr_t gen_params_fn_ptr_;

    const size_t nn_output_size_;
};


#endif  // __MEML_INTERFACE_HPP__