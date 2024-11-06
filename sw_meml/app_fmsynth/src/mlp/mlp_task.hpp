#ifndef __MLP_TASK_HPP__
#define __MLP_TASK_HPP__

#include "../chans_and_data.h"
#include <vector>
#include <cstdint>
#include <cstddef>

extern "C" {
    #include <xcore/chanend.h>

    void mlp_init(chanend_t nn_paramupdate);
    void mlp_inference_nochannel(ts_joystick_read joystick_read);

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

class Dataset {
 public:
    static void Add(std::vector<float> &feature, std::vector<float> &label);
    static void Train();
    static void Clear();
    static void Load(std::vector< std::vector<float> > &features,
                     std::vector< std::vector<float> > &labels);
    static void Fetch(const std::vector< std::vector<float> > *features,
                     const std::vector< std::vector<float> > *labels);
};


#endif  // __MLP_TASK_HPP__