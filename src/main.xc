// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// XMOS Public License: Version 1

#include "platform.h"

extern "C" {
void main_tile0(chanend c0, chanend c1);
void main_tile1(chanend c0, chanend c1);
#if (XSCOPE_HOST_IO_ENABLED == 1)
#ifndef XSCOPE_HOST_IO_TILE
#define XSCOPE_HOST_IO_TILE 0
#endif
void init_xscope_host_data_user_cb(chanend c_host);
#endif
}

int main(void) {

#if (XSCOPE_HOST_IO_ENABLED == 1)
  chan c_xscope_host;
#endif
  chan c_tile0_tile1;
  chan c_tile1_tile0;

  par {

#if (XSCOPE_HOST_IO_ENABLED == 1)
    xscope_host_data(c_xscope_host);
    on tile[XSCOPE_HOST_IO_TILE] : init_xscope_host_data_user_cb(c_xscope_host);
#endif

    on tile[0] : main_tile0(c_tile0_tile1, c_tile1_tile0);
    on tile[1] : main_tile1(c_tile1_tile0, c_tile0_tile1);

  }  // par

  return 0;
}
