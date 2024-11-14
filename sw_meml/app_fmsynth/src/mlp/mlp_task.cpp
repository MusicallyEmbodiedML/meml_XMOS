#include "mlp_task.hpp"
#include "MLP.h"
#include "Data.h"
#include "Dataset.hpp"
#include "utils/Flash.hpp"
#include "utils/Serialise.hpp"
#include "../chans_and_data.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cassert>

extern "C" {
    #include <xcore/channel.h>
}


// Private "methods"
static void mlp_load_all();
static void mlp_save_all();


// MLP config constants
static const unsigned int kBias = 1;
static const std::vector<std::string> layers_activfuncs = {
    "relu", "relu", "relu", "sigmoid"
};
static const bool use_constant_weight_init = false;
static const float constant_weight_init = 0;

// MLP memory
static MLP<float> *mlp_ = nullptr;
static char mlp_mem_[sizeof(MLP<float>)];
static size_t n_output_params_ = 0;
static MLP<float>::mlp_weights mlp_stored_weights_;
std::vector<float> mlp_stored_output;
bool randomised_state_ = false;
bool redraw_weights_ = true;

// Flash memory
static XMOSFlash *flash_ = nullptr;
static char flash_mem_[sizeof(XMOSFlash)];
static constexpr size_t kSigLength = 4;
static const std::vector<uint8_t> payload_signature_ref = {'b', 'e', 't', 'a'};

/******************************
 * MLP TASK
 ******************************/

chanend_t nn_paramupdate_ = 0;


void mlp_init(chanend_t nn_paramupdate, size_t n_params)
{
    const std::vector<size_t> layers_nodes = {
        sizeof(ts_joystick_read)/sizeof(float) + kBias,
        10, 10, 14,
        n_params
    };

    n_output_params_ = n_params;

    // Instantiate objects
    mlp_ = new (mlp_mem_) MLP<float>(
        layers_nodes,
        layers_activfuncs,
        "mse",
        use_constant_weight_init,
        constant_weight_init
    );
    flash_ = new(flash_mem_) XMOSFlash();

    // Instantiate channels
    nn_paramupdate_ = nn_paramupdate;

    std::printf("MLP- Initialised.\n");

    mlp_load_all();
}

void mlp_train()
{
    // Restore weights first
    if (randomised_state_ && mlp_stored_weights_.size() > 0) {
        mlp_->SetWeights(mlp_stored_weights_);
        randomised_state_ = false;
        std::printf("MLP- Restored pre-random weights.\n");
    }

    MLP<float>::training_pair_t dataset(Dataset::GetFeatures(), Dataset::GetLabels());

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
              0.0001,
              false);
    std::printf("MLP- Trained.\n");

    mlp_save_all();
}


void mlp_load_all()
{
    flash_->connect();
    flash_->ReadFromFlash();
    flash_->disconnect();

    size_t r_head = 0;
    const std::vector<uint8_t> *flash_buffer_ptr = flash_->GetPayloadPtr();

    // Check that data was formatted correctly
    bool data_found = false;
    if (flash_buffer_ptr->size() > kSigLength) {
        std::vector<uint8_t> payload_signature(flash_buffer_ptr->begin(),
                                            flash_buffer_ptr->begin() + kSigLength);
        data_found = payload_signature == payload_signature_ref;
        r_head += kSigLength;
    }

    if (data_found) {
        Dataset::GetFeatures().clear();
        r_head = Serialise::ToVector2D(r_head, *flash_buffer_ptr, Dataset::GetFeatures());
        Dataset::GetLabels().clear();
        r_head = Serialise::ToVector2D(r_head, *flash_buffer_ptr, Dataset::GetLabels());
        r_head = mlp_->FromSerialised(r_head, *flash_buffer_ptr);
        std::printf("MLP- TODO print loaded data info\n");
    } else {
        std::printf("MLP- No flash data found.\n");
    }
}


void mlp_save_all()
{
    // Put in signature
    size_t w_head = 0;
    std::vector<uint8_t> flash_buffer(payload_signature_ref.begin(),
                                      payload_signature_ref.end());
    w_head += kSigLength;

    // Serialise data
    w_head = Serialise::FromVector2D(w_head, Dataset::GetFeatures(), flash_buffer);
    w_head = Serialise::FromVector2D(w_head, Dataset::GetLabels(), flash_buffer);
    w_head = mlp_->Serialise(w_head, flash_buffer);

    // Dump data to buffer
    flash_->SetPayload(flash_buffer);
    flash_->connect();
    flash_->WriteToFlash();
    flash_->disconnect();
    std::printf("MLP- Dataset and model saved to flash.\n");
    flash_->connect();
    flash_->ReadFromFlash();
    flash_->disconnect();
    auto flash_buffer_ptr = flash_->GetPayloadPtr();
    if (*flash_buffer_ptr == flash_buffer) {
        std::printf("MLP- Flash write verified.\n");
    } else {
        std::printf("MLP- Flash write failed!\n");
    }
}

void mlp_draw(float speed)
{
    //MLP<float>::mlp_weights pre_randomised_weights_(mlp_->GetWeights());
    //assert(&(pre_randomised_weights_[0][0]) != &(mlp_->m_layers[0].m_nodes[0].m_weights));

    if (!randomised_state_) {
        mlp_stored_weights_ = mlp_->GetWeights();
        randomised_state_ = true;
        std::printf("MLP- Stored pre-random weights.\n");
        redraw_weights_ = true;
    }
    if (redraw_weights_) {
        mlp_->DrawWeights();
        std::printf("MLP- Weights randomised.\n");
        redraw_weights_ = false;
    } else {
        mlp_->MoveWeights(speed);
        std::printf("MLP- Weights moved %f%%.\n", speed*100.);
    }

    // Check weights are actually any different
    // MLP<float>::mlp_weights post_randomised_weights_(mlp_->GetWeights());
    // std::vector<float>& pre_random(pre_randomised_weights_[0][0]);
    // std::vector<float>& post_random(post_randomised_weights_[0][0]);
    // for (unsigned int n = 0; n < pre_random.size(); n++) {
    //     std::printf("Pre: %f --- Post: %f\n", pre_random[n], post_random[n]);
    // }
}

void mlp_trigger_redraw()
{
    redraw_weights_ = true;
}


void mlp_inference_nochannel(ts_joystick_read joystick_read) {
    // Instantiate data in/out
    std::vector<num_t> input{
        joystick_read.potX,
        joystick_read.potY,
        joystick_read.potRotate,
        1.f  // bias
    };
    std::vector<num_t> output(n_output_params_);
    // Run model
    mlp_->GetOutput(input, &output);

    // TODO apply transform here if set

    mlp_stored_output = output;

    // Send result
    //std::printf("INTF- Sending paramupdate to FMsynth...\n");
    chan_out_buf_byte(
        nn_paramupdate_,
        reinterpret_cast<uint8_t *>(output.data()),
        sizeof(num_t) * n_output_params_
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
        std::vector<num_t> output(n_output_params_);
        // Run model
        mlp_->GetOutput(input, &output);

        // Send result
        chan_out_buf_byte(
            nn_paramupdate,
            reinterpret_cast<uint8_t *>(output.data()),
            sizeof(num_t) * n_output_params_
        );

    }  // while(true)
}
