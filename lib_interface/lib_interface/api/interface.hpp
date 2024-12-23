#ifndef __INTERFACE_WRAPPER_HPP__
#define __INTERFACE_WRAPPER_HPP__

#include "MEMLInterface.hpp"

///
// C API
///

extern MEMLInterface *meml_interface;

extern "C" {

#include <xcore/chanend.h>


void interface_init(
    chanend_t interface_fmsynth);

void interface_init_with_midi(
    chanend_t interface_fmsynth,
    chanend_t interface_midi);

void interface_init_with_pulse(
    chanend_t interface_fmsynth,
    chanend_t interface_pulse);

}  // extern "C"


#endif  // __INTERFACE_WRAPPER_HPP__