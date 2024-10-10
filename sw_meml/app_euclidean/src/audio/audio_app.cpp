#include "audio_app.h"

#include <vector>
#include <cmath>
#include <cstdio>

extern "C" {
    #include <xcore/channel.h>
    #include <xcore/channel_streaming.h>
    #include <xscope.h>
    #include "xassert.h"
}

#include "../chans_and_data.h"
#include "audio_buffers.h"
// Include audio components
#include "Phasor.hpp"


static port_t port1, port2;
static size_t port1Out=0, port2Out=0;

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

//FMSynth *fmsyn = nullptr;
//static char fmsyn_mem_[sizeof(FMSynth)];

static EuclideanSeq *seq[kNGenerators] = { nullptr };
static char seq_mem_[sizeof(EuclideanSeq) * kNGenerators];

static Phasor *ph = nullptr;
static char ph_mem_[sizeof(Phasor)];

/**************************
 * MAIN ROUTINES
 **************************/


void audio_app_init(float sample_rate, port_t p1, port_t p2)
{
    sample_buffer.resize(kAudioChannels);
    for (unsigned int ch = 0; ch < kAudioChannels; ch++) {
        sample_buffer[ch].resize(kAudioSamples, 0);
    }

    port_enable(p1);

    // Component inits
    // fmsyn = new (fmsyn_mem_) FMSynth(sample_rate);
    ph = new (ph_mem_) Phasor(sample_rate);
    ph->SetF0(0.5);

    // Generators
    // EuclideanSeq::params seq_params[kNGenerators] = {
    //     {
    //         11, // n
    //         7,  // k
    //         0,  // offset_n
    //         1,  // offset_d
    //     },
    //     {
    //         7, // n
    //         4,  // k
    //         3,  // offset_n
    //         8,  // offset_d
    //     }
    // };
    for (unsigned int n = 0; n < kNGenerators; n++) {
        seq[n] = new (seq_mem_ + n * sizeof(EuclideanSeq)) EuclideanSeq();
        // Do not set params until NN sends them
        //seq[n]->SetParams(seq_params[n]);
    }

    // Ports
    port1 = p1;
    port2 = p2;
}

static unsigned int counter = 0;

void audio_loop(chanend_t i2s_audio_in)
{
    while (1) {
        size_t audio_buf_idx = s_chan_in_word(i2s_audio_in);
        xscope_int(0, 1);
        audio_buffer_ptr_t audio_buf = audio_buffer_ptrs[audio_buf_idx];
        to_float_buf(audio_buf, sample_buffer);

        //port_out(port1, 0xf);
        //port_out(port2, 0x1);

        // Floating-point processing here

        for (unsigned int smp = 0; smp < kAudioSamples; smp++) {

            if (counter++ >= 48000) {
                std::printf(".\n");
                counter = 0;
            }
            float phasor = ph->Process();
            //xscope_float(1, phasor);

            // Generate     
            for(size_t i_generator=0; i_generator < kNGenerators; i_generator++) {
                bool euclideanSig = seq[i_generator]->Process(phasor);
                // if (i_generator == 0) {
                //     xscope_float(2, seq[0]->Probe(0));
                //     xscope_float(3, seq[0]->Probe(1));
                //     xscope_int(4, euclideanSig);
                // }

                // Port outs
                if (i_generator == 7) {
                    port2Out = euclideanSig;
                }else{
                    if (euclideanSig) {
                        //on
                        port1Out |= (0x1 << i_generator);
                    }else{
                        //off
                        port1Out &= (~(0x1 << i_generator));
                    }
                }

                // Port output
                port_out(port1, port1Out);
                port_out(port2, port2Out);

            }
        }


        // Output
        from_float_buf(sample_buffer, audio_buf);
        xscope_int(0, 0);
    }
}


void audio_app_paramupdate(chanend_t fmsynth_paramupdate)
{
    std::vector<num_t> params(kN_gen_params);

    while (true) {
#if 1
        chan_in_buf_byte(
            fmsynth_paramupdate,
            reinterpret_cast<unsigned char *>(params.data()),
            sizeof(num_t) * kN_gen_params
        );

        for (unsigned int n_gen = 0; n_gen < kNGenerators; n_gen++) {
            std::vector<num_t>::iterator start = params.begin() + n_gen*EuclideanSeq::n_params;
            std::vector<num_t>::iterator end = start + EuclideanSeq::n_params;
            std::vector<num_t> single_gen_params(start, end);
            assert(single_gen_params.size() == EuclideanSeq::n_params);

            seq[n_gen]->MapNNParams(single_gen_params);
        }

#endif
    }
}

void audio_app_pulseupdate(chanend_t interface_pulse)
{
    while (1) {
        int32_t pulse = chan_in_word(interface_pulse);
        //std::printf("AUD- Pulse=%ld\n", pulse);
        float pulse_seconds = static_cast<float>(pulse)*1e-6;
        float pulse_freq = 1./pulse_seconds;
        //std::printf("AUD- Pulse Freq(Hz)=%f\n", pulse_freq);
        ph->SetF0(pulse_freq);
    }
}