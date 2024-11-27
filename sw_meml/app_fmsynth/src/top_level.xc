// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

#include <platform.h>
#include <xs1.h>
#include "i2s.h"
#include <stdio.h>

#include "audio/audio_app.h"
#include "codec_setup.h"
#include "audio/audio_buffers.h"
#include "chans_and_data.h"

#ifndef XMOS_I2S_MASTER
#define XMOS_I2S_MASTER         1
#endif

// UART resources
on tile[0]: out port p_uart_tx =                            XS1_PORT_4C;
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
extern void audio_app_init(float sample_rate);
extern void audio_app_paramupdate(chanend fmsynth_paramupdate);
extern void audio_app_midi(chanend interface_midi);
// UART tasks
extern void uart_init();
extern void uart_rx_task();
extern void uart_tx_task([[hwtimer]] timer testsend_timer);
// MLP tasks
extern void mlp_init(chanend interface_fmsynth, size_t n_params);
// Interface
extern void interface_init(chanend interface_fmsynth, chanend interface_midi);


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
    chan interface_midi;
    par{
        on tile[0]: {
            // Tile 0 channels

            // Tile 0 timers
            [[hwtimer]]
            timer testsend_timer;

            // Init tasks (in reverse call order)
            mlp_init(interface_fmsynth, kN_nn_params);
            interface_init(
                interface_fmsynth,
                interface_midi
            );
            uart_init();
            // Runtime tasks
            par {
                tile_0_main(i2s_remote);
                uart_rx_task();
                uart_tx_task(testsend_timer);
            }
        }
        on tile[1]: {
            interface i2s_frame_callback_if i_i2s;
            streaming chan audio_in, audio_out;
            tile_1_main(i2s_remote);
            audio_app_init(SAMPLE_FREQUENCY);
            par {
                audio_loop(audio_in);
                i2s_loopback(i_i2s, audio_in);
                i2s_frame_master(i_i2s, p_dac, NUM_I2S_LINES, p_adc, NUM_I2S_LINES, DATA_BITS, p_bclk, p_lrclk, p_mclk, bclk);
                audio_app_paramupdate(interface_fmsynth);
                audio_app_midi(interface_midi);
            }
        }
    }

    return 0;
}
