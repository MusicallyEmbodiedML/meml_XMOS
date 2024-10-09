#ifndef __AUDIO_APP_H__
#define __AUDIO_APP_H__

#if !defined(__XC__)

#if __cplusplus
extern "C" {
#endif

#include <xcore/chanend.h>
#include <xcore/port.h>

extern void audio_app_init(float sample_rate, port_t p1, port_t p2);
extern void audio_loop(chanend_t i2s_audio_in);
extern void audio_app_paramupdate(
    chanend_t fmsynth_paramupdate
);

typedef float sample_t;

#if __cplusplus
}

#include "EuclideanSeq.hpp"
const size_t kNGenerators = 4;
const size_t kN_gen_params = kNGenerators * EuclideanSeq::n_params;

#endif  // __cplusplus

#endif  // __XC__

#endif  // __AUDIO_APP_H__