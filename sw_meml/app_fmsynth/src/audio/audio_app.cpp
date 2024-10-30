#include "audio_app.h"

#include <vector>
#include <cmath>
#include <cstdio>

extern "C" {
    #include <xcore/channel.h>
    #include <xcore/channel_streaming.h>
    #include <xscope.h>
}

#include "../chans_and_data.h"
#include "audio_buffers.h"
// Include audio components
#include "FMSynth.hpp"


/**************************
 * STATIC HELPER FUNCTIONS
 **************************/


template<typename T>
void clear_buffer(T *buffer, size_t length) {
    memset(buffer, 0, sizeof(T)*length);
}

template<typename T>
void copy_buffer(T *src, T *dst, size_t length) {
    memcpy(dst, src, sizeof(T)*length);
}

void to_float_buf(audio_buffer_ptr_t &src, std::vector< std::vector<sample_t> > &dst) {

    static const sample_t scaling = std::pow(2.f, -31);

    for(unsigned int ch = 0; ch < kAudioChannels; ch++) {
        auto &buffer_ch = dst[ch];
        for (unsigned int smp = 0; smp < kAudioSamples; smp++) {
            buffer_ch[smp] = static_cast<sample_t>(src[ch][smp]) * scaling;
            //if (ch == 0) xscope_float(1, dst[ch][smp]);
        }
    }
}

void from_float_buf(std::vector< std::vector<sample_t> > &src, audio_buffer_ptr_t &dst) {
    static const sample_t scaling = std::pow(2.f, 31);

    for(unsigned int ch = 0; ch < kAudioChannels; ch++) {
        const auto &buffer_ch = src[ch];
        for (unsigned int smp = 0; smp < kAudioSamples; smp++) {
            dst[ch][smp] = static_cast<int32_t>(buffer_ch[smp] * scaling);
            //if (ch == 0) xscope_float(3, buffer_ch[smp]);
        }
    }
}

static std::vector< std::vector<sample_t> > sample_buffer;

/**************************
 * OBJECTS THAT MAKE SOUND
 **************************/

FMSynth *fmsyn = nullptr;
static char fmsyn_mem_[sizeof(FMSynth)];


/**************************
 * MAIN ROUTINES
 **************************/


void audio_app_init(float sample_rate)
{
    sample_buffer.resize(kAudioChannels);
    for (unsigned int ch = 0; ch < kAudioChannels; ch++) {
        sample_buffer[ch].resize(kAudioSamples, 0);
    }

    // Component inits
    fmsyn = new (fmsyn_mem_) FMSynth(sample_rate);
}


void audio_loop(chanend_t i2s_audio_in)
{
    while (1) {
        size_t audio_buf_idx = s_chan_in_word(i2s_audio_in);
        xscope_int(0, 1);
        audio_buffer_ptr_t audio_buf = audio_buffer_ptrs[audio_buf_idx];
        to_float_buf(audio_buf, sample_buffer);

        // Floating-point processing here

        for (unsigned int smp = 0; smp < kAudioSamples; smp++) {
            float y;
            if (fmsyn != nullptr) {
                y = fmsyn->process();
            } else {
                y = 0;
            }
            for(unsigned int ch = 0; ch < kAudioChannels; ch++) {
                sample_buffer[ch][smp] = y;
            //if (ch == 0) xscope_float(1, sample_buffer[ch][smp]);
            }
        }


        // Output
        from_float_buf(sample_buffer, audio_buf);
        xscope_int(0, 0);
    }
}


void audio_app_paramupdate(chanend_t fmsynth_paramupdate)
{
    std::vector<num_t> params(kN_synthparams);

    while (true) {
#if 1
        chan_in_buf_byte(
            fmsynth_paramupdate,
            reinterpret_cast<unsigned char *>(params.data()),
            sizeof(num_t) * kN_synthparams
        );

        //debug_printf("FMSynth- Something is received.\n");

        if (fmsyn != nullptr) {
            fmsyn->mapParameters(params);
            //std::printf("FMSynth- Params are mapped.\n");
        }
#endif
    }
}