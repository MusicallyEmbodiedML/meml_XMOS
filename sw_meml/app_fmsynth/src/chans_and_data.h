#ifndef __CHANS_AND_DATA_H__
#define __CHANS_AND_DATA_H__


#if defined(__cplusplus) || defined(__XC__)
extern "C" {
#endif

#include <stddef.h>

/**
 * @brief Inserts
 * 
 */
typedef struct {
    float potX;
    float potY;
    float potRotate;
} ts_joystick_read;

typedef enum {
    joystick_potX,
    joystick_potY,
    joystick_potRotate,
    joystick_nPots
} te_joystick_pot;

typedef enum {
    toggle_training,
    button_randomise,
    toggle_savedata,
    button_reset,
    toggle_discretise,
    toggle_complex,
    button_nButtons
} te_button_idx;

typedef enum {
    slider_randomSpeed,
    slider_nSliders
} te_slider_idx;

typedef enum {
    mode_inference,
    mode_training
} te_nn_mode;

const size_t kN_nn_params = 14;

#if defined(__cplusplus) || defined(__XC__)
}
#endif

#endif //__CHANS_AND_DATA_H__