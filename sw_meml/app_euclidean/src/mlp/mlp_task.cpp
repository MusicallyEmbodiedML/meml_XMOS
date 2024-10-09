#include "mlp_task.hpp"
#include "MLP.h"
#include "../chans_and_data.h"
#include "../audio/audio_app.h"

#include <memory>
#include <algorithm>
#include <cmath>
#include <cstdio>

extern "C" {
    #include <xcore/channel.h>
}


// MLP config constants
static const unsigned int kBias = 1;
static const std::vector<size_t> layers_nodes = {
    sizeof(ts_joystick_read)/sizeof(float) + kBias,
    10, 10, 14,
    kN_gen_params
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
    std::printf("MLP- Added example.\n");
    std::printf("MLP- Feature size %d, label size %d.\n", features.size(), labels.size());
}

void Dataset::Train()
{
    MLP<float>::training_pair_t dataset(features, labels);

    std::printf("MLP- Feature size %d, label size %d.\n", dataset.first.size(), dataset.second.size());
    if (!dataset.first.size() || !dataset.second.size()) {
        return;
    }
    std::printf("MLP- Feature dim %d, label dim %d.\n", dataset.first[0].size(), dataset.second[0].size());
    if (!dataset.first[0].size() || !dataset.second[0].size()) {
        return;
    }
    std::printf("MLP- Training...\n");
    mlp_->Train(dataset,
              1.,
              1000,
              0.00001,
              false);
    std::printf("MLP- Trained.\n");
}


/******************************
 * MLP TASK
 ******************************/

chanend_t nn_paramupdate_ = 0;

void mlp_init(chanend_t nn_paramupdate)
{
    mlp_ = new (mlp_mem_) MLP<float>(
        layers_nodes,
        layers_activfuncs,
        "mse",
        use_constant_weight_init,
        constant_weight_init
    );

    nn_paramupdate_ = nn_paramupdate;

    std::printf("MLP- Initialised.\n");
}


void mlp_inference_nochannel(ts_joystick_read joystick_read) {
    // Instantiate data in/out
    std::vector<num_t> input{
        joystick_read.potX,
        joystick_read.potY,
        joystick_read.potRotate,
        1.f  // bias
    };
    std::vector<num_t> output(kN_gen_params);
    // Run model
    mlp_->GetOutput(input, &output);

    // Send result
    //std::printf("INTF- Sending paramupdate to FMsynth...\n");
    chan_out_buf_byte(
        nn_paramupdate_,
        reinterpret_cast<uint8_t *>(output.data()),
        sizeof(num_t) * kN_gen_params
    );
}


void mlp_inference_task(chanend_t dispatcher_nn,
                        chanend_t nn_paramupdate,
                        chanend_t nn_data,
                        chanend_t nn_train)
{
    // Init
    std::unique_ptr<ts_joystick_read> joystick_read;

     while (true) {
        std::printf("NN- Waiting for data from channel 0x%x...\n", dispatcher_nn);
        // Blocking acquisition of pot data from dispatcher_nn
        chan_in_buf_byte(
            dispatcher_nn,
            reinterpret_cast<uint8_t *>(joystick_read.get()),
            sizeof(ts_joystick_read)
        );
        std::printf("NN- Received joystick read in NN task.\n");

        // Instantiate data in/out
        std::vector<num_t> input{
            joystick_read->potX,
            joystick_read->potY,
            joystick_read->potRotate,
            1.f  // bias
        };
        std::vector<num_t> output(kN_gen_params);
        // Run model
        mlp_->GetOutput(input, &output);

        // Send result
        chan_out_buf_byte(
            nn_paramupdate,
            reinterpret_cast<uint8_t *>(output.data()),
            sizeof(num_t) * kN_gen_params
        );

    }  // while(true)
}
