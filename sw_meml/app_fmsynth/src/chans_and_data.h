#ifndef __CHANS_AND_DATA_H__
#define __CHANS_AND_DATA_H__


#if defined(__cplusplus) || defined(__XC__)
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

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
    mode_training,
    mode_nModes
} te_nn_mode;

typedef enum {
    expl_mode_nnweights,
    expl_mode_pretrain,
    expl_mode_zoom,
    expl_nModes
} te_expl_mode;


typedef enum {
    app_id_fmsynth,
    app_id_euclidean,
    app_id_multifx,
    app_id_machinelisten,
    app_nIDs
} te_app_id;


typedef struct {

    // 32-bit parameters
    uint32_t n_iterations;
    float last_error;
    float exploration_range;

    // 8-bit parameters
    te_app_id app_id;
    uint8_t current_dataset;
    uint8_t current_model;
    te_nn_mode current_nn_mode;
    te_expl_mode current_expl_mode;

} ts_app_state;

extern ts_app_state gAppState;
extern const size_t kApp_MaxModels;
extern const size_t kApp_MaxDatasets;
extern const size_t kN_nn_params;


#if defined(__cplusplus) || defined(__XC__)
}
#endif

#endif //__CHANS_AND_DATA_H__