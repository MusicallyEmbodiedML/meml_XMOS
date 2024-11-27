#include "interface.hpp"
#include "MEMLInterface.hpp"
#include "FMSynth.hpp"

extern "C" {
   #include <xcore/channel.h>
}

///
// C WRAPPER TASK
///

static char meml_interface_mem_[sizeof(MEMLInterface)];
MEMLInterface *meml_interface = nullptr;

MEML_IF_CALLBACK_ATTR void interface_gen_callback(std::vector<float> &joystick) {
    FMSynth::GenParams(joystick);
}


void interface_init(chanend_t interface_fmsynth)
{
    meml_interface = new (meml_interface_mem_) MEMLInterface(
        interface_fmsynth,
        static_cast<chanend_t>(0),
        &(interface_gen_callback),
        kN_synthparams
    );
}


void interface_init_with_midi(chanend_t interface_fmsynth, chanend_t interface_midi)
{
    meml_interface = new (meml_interface_mem_) MEMLInterface(
        interface_fmsynth,
        interface_midi,
        &(interface_gen_callback),
        kN_synthparams
    );
    meml_interface->EnableMIDI();
}