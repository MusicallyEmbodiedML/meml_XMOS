// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <platform.h>
#include <xs1.h>
#include "i2s.h"
#include <stdio.h>

#include "audio/audio_app.h"
#include "codec_setup.h"
#include "audio/audio_buffers.h"

#ifndef XMOS_I2S_MASTER
#define XMOS_I2S_MASTER         1
#endif

//GPIO for pulse generators
on tile[1]: out port p_pulse1 =   XS1_PORT_4B;
on tile[1]: out port p_pulse2 =   XS1_PORT_1M;


// I2S resources
on tile[1]: in port p_mclk =                                PORT_MCLK_IN;
on tile[1]: buffered out port:32 p_lrclk =                  PORT_I2S_LRCLK;
on tile[1]: out port p_bclk =                               PORT_I2S_BCLK;
on tile[1]: buffered out port:32 p_dac[NUM_I2S_LINES] =     {PORT_I2S_DAC_DATA};
on tile[1]: buffered in port:32 p_adc[NUM_I2S_LINES] =      {PORT_I2S_ADC_DATA};
on tile[1]: clock bclk =                                    XS1_CLKBLK_1;

// I2S tasks
extern void tile_0_main(chanend c);
extern void tile_1_main(chanend c);
// Audio tasks
extern void audio_loop(streaming chanend);
extern void audio_app_init(float sample_rate, out port p1, out port p2);
extern void audio_app_paramupdate(chanend fmsynth_paramupdate);
// UART tasks
extern void uart_init();
extern void uart_rx_task();
// MLP tasks
extern void mlp_init(chanend interface_fmsynth);
extern void mlp_inference_task(chanend interface_nn_joystick,
                               chanend interface_fmsynth,
                               chanend interface_nn_data,
                               chanend interface_nn_train);
// Interface
extern void interface_init(chanend interface_nn_joystick,
                           chanend interface_fmsynth,
                           chanend interface_nn_data,
                           chanend interface_nn_train);


/**
 * I2S Loopback ISR
 */

void i2s_loopback(server i2s_frame_callback_if i_i2s, streaming chanend audio_in)
{
    unsigned char sample_sent = 0;
    size_t audio_buf_cur_sample_i2s = 0;

    while (1) {
    select {
        case i_i2s.init(i2s_config_t &?i2s_config, tdm_config_t &?tdm_config):
            i2s_config.mode = I2S_MODE_I2S;
            i2s_config.mclk_bclk_ratio = (MASTER_CLOCK_FREQUENCY / (SAMPLE_FREQUENCY * CHANS_PER_FRAME * DATA_BITS));
            break;

        case i_i2s.receive(size_t n_chans, int32_t in_samps[n_chans]):
            if (sample_sent) {
                unsafe {
                    audio_buffer_ptr_t ab_ptr = audio_buffer_ptrs[audio_buffer_current_idx];
                    // Receive frame from I2S in
                    for (int i = 0; i < n_chans; i++){
                        ab_ptr[i][audio_buf_cur_sample_i2s] = in_samps[i];
                    }
                }
                /* If buffer is full, signal which buffer is available
                to audio loop and switch to other buffer */
                audio_buf_cur_sample_i2s++;
                if (audio_buf_cur_sample_i2s >= kAudioSamples) {
                    audio_buf_cur_sample_i2s = 0;
                    audio_in <: audio_buffer_current_idx;
                    audio_buffer_current_idx = (audio_buffer_current_idx) ? 0 : 1;
                }
                sample_sent = 0;
            }
            break;

        case i_i2s.send(size_t n_chans, int32_t out_samps[n_chans]):
            if (!sample_sent) {
                unsafe {
                    audio_buffer_ptr_t ab_ptr = audio_buffer_ptrs[audio_buffer_current_idx];
                    // Send frame to I2S out
                    for (int i = 0; i < n_chans; i++){
                        out_samps[i] = ab_ptr[i][audio_buf_cur_sample_i2s];
                    }
                }
                sample_sent = 1;
            }
            break;

        case i_i2s.restart_check() -> i2s_restart_t restart:
            restart = I2S_NO_RESTART; // Keep on looping
            break;
        }
    }
}

/**
 * MAIN routine
 */


int main(void){
    // Inter-tile channels
    chan i2s_remote;
    chan interface_fmsynth;
    par{
        on tile[0]: {
            // Tile 0 channels
            chan interface_nn_joystick;
            chan interface_nn_data;
            chan interface_nn_train;

            // Init tasks
            uart_init();
            mlp_init(interface_fmsynth);
            interface_init(
                interface_nn_joystick,
                interface_fmsynth,
                interface_nn_data,
                interface_nn_train
            );
            // Runtime tasks
            par {
                tile_0_main(i2s_remote);
                uart_rx_task();
            }
        }
        on tile[1]: {
            interface i2s_frame_callback_if i_i2s;
            streaming chan audio_in, audio_out;
            tile_1_main(i2s_remote);
            audio_app_init(SAMPLE_FREQUENCY, p_pulse1, p_pulse2);
            par {
                audio_loop(audio_in);
                i2s_loopback(i_i2s, audio_in);
                i2s_frame_master(i_i2s, p_dac, NUM_I2S_LINES, p_adc, NUM_I2S_LINES, DATA_BITS, p_bclk, p_lrclk, p_mclk, bclk);
                audio_app_paramupdate(interface_fmsynth);
            }
        }
    }

    return 0;
}
