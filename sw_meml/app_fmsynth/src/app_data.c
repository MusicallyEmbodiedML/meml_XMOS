#include "chans_and_data.h"

const size_t kN_nn_params = 14;


// TODO AM this should be persistend and in flash
ts_app_state gAppState = {
    /* n_iterations: */ 500,
    /* last_error: */ 0,
    /* exploration_range: */ 1.0f,
    /* app_id: */ app_id_fmsynth,
    /* current_dataset: */ 0,
    /* current_model: */ 0,
    /* current_nn_mode: */ mode_inference,
    /* current_expl_mode: */ expl_mode_pretrain
};
