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

// UART resources
on tile[0] : port p_uart_rx = PORT_UART_RX;
// I2S resources
on tile[1]: in port p_mclk =                                PORT_MCLK_IN;
on tile[1]: buffered out port:32 p_lrclk =                  PORT_I2S_LRCLK;
on tile[1]: out port p_bclk =                               PORT_I2S_BCLK;
on tile[1]: buffered out port:32 p_dac[NUM_I2S_LINES] =     {PORT_I2S_DAC_DATA};
on tile[1]: buffered in port:32 p_adc[NUM_I2S_LINES] =      {PORT_I2S_ADC_DATA};
on tile[1]: clock bclk =                                    XS1_CLKBLK_1;


extern void tile_0_main(chanend c);
extern void tile_1_main(chanend c);
extern void audio_loop(streaming chanend);
extern void audio_app_init();
extern void uart_rx_task();
extern void uart_init_task();

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

int main(void){
    chan c;

    par{
        on tile[0]: {
            uart_init_task();
            par {
                tile_0_main(c);
                uart_rx_task();
            }
        }
        on tile[1]: {
            interface i2s_frame_callback_if i_i2s;
            streaming chan audio_in, audio_out;
            tile_1_main(c);
            audio_app_init();
            par {
                audio_loop(audio_in);
                i2s_loopback(i_i2s, audio_in);
                i2s_frame_master(i_i2s, p_dac, NUM_I2S_LINES, p_adc, NUM_I2S_LINES, DATA_BITS, p_bclk, p_lrclk, p_mclk, bclk);
            }
        }
    }

    return 0;
}
