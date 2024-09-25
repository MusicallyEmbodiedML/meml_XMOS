// Copyright 2021 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.

extern "C" {
#include <xcore/chanend.h>

#include <stdio.h>

void main_tile0(chanend_t c_tile0_tile1, chanend_t c_tile1_tile0);
void main_tile1(chanend_t c_tile1_tile0, chanend_t c_tile0_tile1);

}


void main_tile0(chanend_t c_tile0_tile1, chanend_t c_tile1_tile0)
{
    (void)c_tile0_tile1;
    (void)c_tile1_tile0;

    printf("Hello from tile 0.\n");
}

void main_tile1(chanend_t c_tile1_tile0, chanend_t c_tile0_tile1)
{
    (void)c_tile1_tile0;
    (void)c_tile0_tile1;

    printf("Hello from tile 1.\n");
}
