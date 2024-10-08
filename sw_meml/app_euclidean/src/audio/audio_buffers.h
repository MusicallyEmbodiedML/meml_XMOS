#ifndef __AUDIO_BUFFERS_H__
#define __AUDIO_BUFFERS_H__

#if defined(__cplusplus) || defined(__XC__)
extern "C" {
#endif

#define kAudioChannels    2
#define kAudioSamples    1
#define kAudioBufferLength    (kAudioChannels*kAudioSamples)

#include <stdint.h>
#include <stddef.h>

typedef int32_t (*audio_buffer_ptr_t)[kAudioSamples];

extern audio_buffer_ptr_t audio_buffer_ptrs[2];

extern size_t audio_buffer_current_idx;

#if defined(__cplusplus) || defined(__XC__)
}
#endif  // extern "C"



#endif  // __AUDIO_BUFFERS_H__