#ifndef __AUDIO_APP_H__
#define __AUDIO_APP_H__

#if !defined(__XC__)

#if __cplusplus
extern "C" {
#endif

#include <xcore/chanend.h>


extern void audio_app_init();
extern void audio_loop(chanend_t i2s_audio_in);

typedef float sample_t;

#if __cplusplus
}
#endif

#endif  // __XC__

#endif  // __AUDIO_APP_H__