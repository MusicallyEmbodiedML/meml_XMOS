#ifndef __MLP_TASK_HPP__
#define __MLP_TASK_HPP__

#include "../chans_and_data.h"
#include <vector>

extern "C" {
    #include <xcore/chanend.h>

    void mlp_init(chanend_t nn_paramupdate, size_t n_params);
    void mlp_inference_nochannel(ts_joystick_read joystick_read);
    void mlp_train();
    void mlp_draw();

    /**
     * @brief Task to handle pot position to FMsynth parameters.
     *
     * @param dispatcher_nn Chanend receiving joystick reads from dispatcher.
     * Expects ts_joystick_read being sent to it.
     * @param nn_paramupdate Chanend sending FM parameters to
     * paramupdate. Sends ts_fmsynth_params.
     */
    void mlp_inference_task(chanend_t dispatcher_nn,
                chanend_t nn_paramupdate,
                chanend_t nn_data,
                chanend_t nn_train);

}  // extern "C"

extern std::vector<float> mlp_stored_output;


#endif  // __MLP_TASK_HPP__