#ifndef __CHANS_AND_DATA_H__
#define __CHANS_AND_DATA_H__

// MLP types
#include "Data.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inserts
 * 
 */
typedef struct {
    num_t potX;
    num_t potY;
    num_t potRotate;
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
    button_savedata,
    button_nButtons
} te_button_idx;

typedef enum {
    mode_inference,
    mode_training
} te_nn_mode;


#ifdef __cplusplus
}
#endif

#endif //__CHANS_AND_DATA_H__