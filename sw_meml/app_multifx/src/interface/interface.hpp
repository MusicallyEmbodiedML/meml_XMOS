#ifndef __INTERFACE_WRAPPER_HPP__
#define __INTERFACE_WRAPPER_HPP__

///
// C API
///

extern "C" {

#include <xcore/chanend.h>


void interface_init(
    chanend_t interface_fmsynth);

}  // extern "C"

///
// C++ API
///

#include "../chans_and_data.h"
#include <cstdint>

class MEMLInterface {
 public:

    MEMLInterface(chanend_t interface_fmsynth);
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
};

extern MEMLInterface *meml_interface;


#endif  // __INTERFACE_WRAPPER_HPP__