// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

extern "C" {
#include <stdio.h>
#include <xcore/channel.h>
#include "xk_evk_xu316/board.h"
#include "codec_setup.h"

void tile_0_main(chanend_t c);
void tile_1_main(chanend_t c);

}

// Board configuration from lib_board_support
static const xk_evk_xu316_config_t hw_config = {
        MASTER_CLOCK_FREQUENCY// default_mclk
};

void tile_0_main(chanend_t c){
    printf("Hello from tile[0]\n");
    xk_evk_xu316_AudioHwRemote(c); // Startup remote I2C master server task
    printf("Bye from tile[0]\n");
}

void tile_1_main(chanend_t c){
    printf("Hello from tile[1]\n");
    xk_evk_xu316_AudioHwChanInit(c);
    xk_evk_xu316_AudioHwInit(&hw_config);
    xk_evk_xu316_AudioHwConfig(SAMPLE_FREQUENCY, hw_config.default_mclk, 0, DATA_BITS, DATA_BITS);
    //chan_out_word(c, AUDIOHW_CMD_EXIT); // Kill the remote config task
    printf("Bye from tile[1]\n");
}
