#include "audio_buffers.h"

static struct {
    int32_t audio_buffer_first[kAudioChannels][kAudioSamples] = { {0} };
    int32_t audio_buffer_second[kAudioChannels][kAudioSamples] = { {0} };
} audio_buffers;

audio_buffer_ptr_t audio_buffer_ptrs[2] = { audio_buffers.audio_buffer_first,
                                            audio_buffers.audio_buffer_second };

size_t audio_buffer_current_idx = 0;
