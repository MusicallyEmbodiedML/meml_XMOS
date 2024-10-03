#ifndef __MLP_TASK_HPP__
#define __MLP_TASK_HPP__

extern "C" {
    #include <xcore/chanend.h>

    void mlp_init();

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


#include <vector>

class Dataset {
 public:
    static void Add(std::vector<float> &feature, std::vector<float> &label);
    static void Train();
};

void DebugDumpJSON();


#endif  // __MLP_TASK_HPP__