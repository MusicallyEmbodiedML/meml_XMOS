#include "mlp_task.hpp"
#include "MLP.h"
#include "../chans_and_data.h"

#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdio>

extern "C" {
    #include <xcore/channel.h>
}

#define MLP_STANDALONE    1
#if MLP_STANDALONE
constexpr size_t kN_synthparams = 14;
#endif


// MLP config constants
static const unsigned int kBias = 1;
static const std::vector<size_t> layers_nodes = {
    sizeof(ts_joystick_read)/sizeof(float) + kBias,
    10, 10, 14,
    kN_synthparams
};
static const std::vector<std::string> layers_activfuncs = {
    "relu", "relu", "relu", "sigmoid"
};
static const bool use_constant_weight_init = false;
static const float constant_weight_init = 0;

// MLP memory
static MLP<float> *mlp_ = nullptr;
static char mlp_mem_[sizeof(MLP<float>)];

/******************************
 * INTERFACE IMPLEMENTATION
 ******************************/

// Static dataset

//static constexpr unsigned int kN_examples = 10;

static std::vector<std::vector<float>> features;//(kN_examples);
static std::vector<std::vector<float>> labels;//(kN_examples);


void Dataset::Add(std::vector<float> &feature, std::vector<float> &label)
{
    auto feature_local = feature;
    auto label_local = label;
    features.push_back(feature_local);
    labels.push_back(label_local);
    std::printf("MLP- Feature size %d, label size %d.\n", features.size(), labels.size());
}

void Dataset::Train()
{
    MLP<float>::training_pair_t dataset(features, labels);

    std::printf("MLP- Feature size %d, label size %d.\n", dataset.first.size(), dataset.second.size());
    std::printf("MLP- Feature dim %d, label dim %d.\n", dataset.first[0].size(), dataset.second[0].size());
    std::printf("MLP- Training...\n");
    mlp_->Train(dataset,
              1.,
              1000,
              0.0001,
              false);
    std::printf("MLP- Trained.\n");
}


/******************************
 * MLP TASK
 ******************************/


void mlp_init()
{
    mlp_ = new (mlp_mem_) MLP<float>(
        layers_nodes,
        layers_activfuncs,
        "mse",
        use_constant_weight_init,
        constant_weight_init
    );

    std::printf("MLP- Initialised.\n");
}

void mlp_inference_task(chanend_t dispatcher_nn,
                        chanend_t nn_paramupdate,
                        chanend_t nn_data,
                        chanend_t nn_train)
{
    // Init
    std::unique_ptr<ts_joystick_read> joystick_read;

     while (true) {
        //std::printf("NN- Waiting for data...\n");
        // Blocking acquisition of pot data from dispatcher_nn
        chan_in_buf_byte(
            dispatcher_nn,
            reinterpret_cast<uint8_t *>(joystick_read.get()),
            sizeof(ts_joystick_read)
        );
        //std::printf("NN- Received joystick read in NN task.\n");

        // Instantiate data in/out
        std::vector<num_t> input{
            joystick_read->potX,
            joystick_read->potY,
            joystick_read->potRotate,
            1.f  // bias
        };
        std::vector<num_t> output(kN_synthparams);
        // Run model
        mlp_->GetOutput(input, &output);

        // Send result
    #if !(MLP_STANDALONE)
        chan_out_buf_byte(
            nn_paramupdate,
            reinterpret_cast<uint8_t *>(output.data()),
            sizeof(num_t) * kN_synthparams
        );
    #endif

    }  // while(true)
}
