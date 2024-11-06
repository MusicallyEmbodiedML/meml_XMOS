#include "mlp_task.hpp"
#include "MLP.h"
#include "utils/Flash.hpp"
#include "utils/Serialise.hpp"
#include "../chans_and_data.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

extern "C" {
    #include <xcore/channel.h>
}

#include "FMSynth.hpp"


static void mlp_load_all();
static void mlp_save_all();


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

// Flash memory
static XMOSFlash *flash_ = nullptr;
static char flash_mem_[sizeof(XMOSFlash)];
static constexpr size_t kSigLength = 4;
static const std::vector<uint8_t> payload_signature_ref = {'b', 'e', 't', 'a'};

/******************************
 * INTERFACE IMPLEMENTATION
 ******************************/

// Static dataset

//static constexpr unsigned int kN_examples = 10;

static std::vector<std::vector<float>> features_;//(kN_examples);
static std::vector<std::vector<float>> labels_;//(kN_examples);


void Dataset::Add(std::vector<float> &feature, std::vector<float> &label)
{
    auto feature_local = feature;
    auto label_local = label;
    features_.push_back(feature_local);
    labels_.push_back(label_local);
    std::printf("MLP- Added example.\n");
    std::printf("MLP- Feature size %d, label size %d.\n", features_.size(), labels_.size());
}

void Dataset::Train()
{
    MLP<float>::training_pair_t dataset(features_, labels_);

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

void Dataset::Clear()
{
    features_.clear();
    labels_.clear();
}

void Dataset::Load(std::vector<std::vector<float>> &features,
                   std::vector<std::vector<float>> &labels)
{
    features_ = features;
    labels_ = labels;
}

void Dataset::Fetch(const std::vector<std::vector<float>> *features,
                    const std::vector<std::vector<float>> *labels)
{
    features = &features_;
    labels = &labels_;
}

/******************************
 * MLP TASK
 ******************************/

chanend_t nn_paramupdate_ = 0;


void mlp_init(chanend_t nn_paramupdate)
{
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
        features_.clear();
        r_head = Serialise::ToVector2D(r_head, *flash_buffer_ptr, features_);
        labels_.clear();
        r_head = Serialise::ToVector2D(r_head, *flash_buffer_ptr, labels_);
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
    w_head = Serialise::FromVector2D(w_head, features_, flash_buffer);
    w_head = Serialise::FromVector2D(w_head, labels_, flash_buffer);
    w_head = mlp_->Serialise(w_head, flash_buffer);

    // Dump data to buffer
    flash_->SetPayload(flash_buffer);
    flash_->connect();
    flash_->WriteToFlash();
    flash_->disconnect();
    std::printf("MLP- Dataset and model saved to flash.\n");
}


void mlp_inference_nochannel(ts_joystick_read joystick_read) {
    // Instantiate data in/out
    std::vector<num_t> input{
        joystick_read.potX,
        joystick_read.potY,
        joystick_read.potRotate,
        1.f  // bias
    };
    std::vector<num_t> output(kN_synthparams);
    // Run model
    mlp_->GetOutput(input, &output);

    // Send result
    //std::printf("INTF- Sending paramupdate to FMsynth...\n");
    chan_out_buf_byte(
        nn_paramupdate_,
        reinterpret_cast<uint8_t *>(output.data()),
        sizeof(num_t) * kN_synthparams
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
        std::vector<num_t> output(kN_synthparams);
        // Run model
        mlp_->GetOutput(input, &output);

        // Send result
        chan_out_buf_byte(
            nn_paramupdate,
            reinterpret_cast<uint8_t *>(output.data()),
            sizeof(num_t) * kN_synthparams
        );

    }  // while(true)
}
