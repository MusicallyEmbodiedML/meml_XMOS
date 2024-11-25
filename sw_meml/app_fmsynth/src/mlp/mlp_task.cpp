#include "mlp_task.hpp"
#include "MLP.h"
#include "Data.h"
#include "Dataset.hpp"
#include "utils/Flash.hpp"
#include "utils/Serialise.hpp"
#include "../chans_and_data.h"
#include "../uart/uart_task.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

extern "C" {
    #include <xcore/channel.h>
    #include "xassert.h"
}


// Enable (and remove) when flash has been updated to support multi-model
#define ENABLE_LEGACY_FLASH    0


// Private "methods"
static void mlp_load_all_();
static void mlp_save_all_();
static void mlp_pretrain_centre_();


// MLP config constants
static const unsigned int kBias = 1;
static const std::vector<std::string> layers_activfuncs = {
    "relu", "relu", "relu", "sigmoid"
};
static const bool use_constant_weight_init = false;
static const float constant_weight_init = 0;
static constexpr ts_joystick_read kZoom_mode_reset { 0.5, 0.5, 0.5 };

// Dataset memory
static char dataset_mem_[kMaxDatasets][sizeof(Dataset)];
static Dataset *dataset_[kMaxDatasets] = { nullptr };
static size_t ds_n_ = 0;

// MLP memory
static MLP<float> *mlp_[kMaxModels] = { nullptr };
static char mlp_mem_[kMaxModels][sizeof(MLP<float>)];
static size_t n_output_params_ = 0;
static MLP<float>::mlp_weights mlp_stored_weights_;
static ts_joystick_read zoom_mode_centre_ = kZoom_mode_reset;
static ts_joystick_read mlp_stored_input = kZoom_mode_reset;
std::vector<float> mlp_stored_output;
static bool randomised_state_ = false;
static bool redraw_weights_ = true;
static bool flag_zoom_in_ = false;
static float speed_ = 1.0f;
static te_expl_mode expl_mode_internal_ = expl_mode_pretrain;
static size_t nn_n_ = 0;

// Flash memory
#if ENABLE_LEGACY_FLASH
static XMOSFlash *flash_ = nullptr;
static char flash_mem_[sizeof(XMOSFlash)];
static constexpr size_t kSigLength = 4;
static const std::vector<uint8_t> payload_signature_ref = {'b', 'e', 't', 'a'};
#endif

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
    xassert(kMaxDatasets == kMaxModels);
    for (unsigned int n = 0; n < kMaxModels; n++) {
        dataset_[n] = new(dataset_mem_[n]) Dataset();
        mlp_[n] = new (mlp_mem_[n]) MLP<float>(
            layers_nodes,
            layers_activfuncs,
            "mse",
            use_constant_weight_init,
            constant_weight_init
        );
    }
#if ENABLE_LEGACY_FLASH
    flash_ = new(flash_mem_) XMOSFlash();
#endif

    // Instantiate channels
    nn_paramupdate_ = nn_paramupdate;

    std::printf("MLP- Initialised.\n");

    mlp_load_all_();
}


static void mlp_trigger_redraw_()
{
    redraw_weights_ = true;
}

void mlp_train()
{
    // Restore weights first
    if (randomised_state_ && mlp_stored_weights_.size() > 0) {
        mlp_[nn_n_]->SetWeights(mlp_stored_weights_);
        randomised_state_ = false;
        std::printf("MLP- Restored pre-random weights.\n");
    }

    MLP<float>::training_pair_t dataset(dataset_[ds_n_]->GetFeatures(), dataset_[ds_n_]->GetLabels());

    std::printf("MLP- Feature size %d, label size %d.\n", dataset.first.size(), dataset.second.size());
    if (!dataset.first.size() || !dataset.second.size()) {
        return;
    }
    std::printf("MLP- Feature dim %d, label dim %d.\n", dataset.first[0].size(), dataset.second[0].size());
    if (!dataset.first[0].size() || !dataset.second[0].size()) {
        return;
    }
    std::printf("MLP- Training for max %lu iterations...\n", gAppState.n_iterations);
    num_t loss = mlp_[nn_n_]->Train(dataset,
              1.,
              gAppState.n_iterations,
              0.0001,
              false);
    std::printf("MLP- Trained.\n");

    mlp_save_all_();

    // Report loss back to state and UI
    gAppState.last_error = loss;
    uart_update_loss(loss);

    flag_zoom_in_ = false;
}


void mlp_load_all_()
{
#if ENABLE_LEGACY_FLASH
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
        dataset_[ds_n_]->GetFeatures().clear();
        r_head = Serialise::ToVector2D(r_head, *flash_buffer_ptr, dataset_[ds_n_]->GetFeatures());
        dataset_[ds_n_]->GetLabels().clear();
        r_head = Serialise::ToVector2D(r_head, *flash_buffer_ptr, dataset_[ds_n_]->GetLabels());
        r_head = mlp_[nn_n_]->FromSerialised(r_head, *flash_buffer_ptr);
        std::printf("MLP- TODO print loaded data info\n");
    } else {
        std::printf("MLP- No flash data found.\n");
    }
#endif
}


void mlp_save_all_()
{
#if ENABLE_LEGACY_FLASH
    // Put in signature
    size_t w_head = 0;
    std::vector<uint8_t> flash_buffer(payload_signature_ref.begin(),
                                      payload_signature_ref.end());
    w_head += kSigLength;

    // Serialise data
    w_head = Serialise::FromVector2D(w_head, dataset_[ds_n_]->GetFeatures(), flash_buffer);
    w_head = Serialise::FromVector2D(w_head, dataset_[ds_n_]->GetLabels(), flash_buffer);
    w_head = mlp_[nn_n_]->Serialise(w_head, flash_buffer);

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
#endif
}

void mlp_draw(float speed)
{
    speed_ = speed;

    if (!randomised_state_) {
        mlp_stored_weights_ = mlp_[nn_n_]->GetWeights();
        randomised_state_ = true;
        std::printf("MLP- Stored pre-random weights.\n");
        mlp_trigger_redraw_();
        flag_zoom_in_ = false;
    }
    if (redraw_weights_) {
        mlp_[nn_n_]->DrawWeights();
        std::printf("MLP- Weights randomised.\n");
        redraw_weights_ = false;
    } else {
        if (expl_mode_internal_ == expl_mode_zoom) {

            flag_zoom_in_ = true;
            zoom_mode_centre_ = mlp_stored_input;

        } else if (expl_mode_internal_ == expl_mode_pretrain) {

            zoom_mode_centre_ = kZoom_mode_reset;
            flag_zoom_in_ = true;
            // Train network with only one data point at the centre
            mlp_pretrain_centre_();
            std::printf("MLP- Pretrained on centre (speed %f%%).\n", speed*100.f);

        } else if (expl_mode_internal_ == expl_mode_nnweights) {

            zoom_mode_centre_ = kZoom_mode_reset;
            // Randomise weights less ("move" by speed)
            mlp_[nn_n_]->MoveWeights(speed);
            std::printf("MLP- Weights moved %f%%.\n", speed*100.f);

        }
    }
}

void mlp_add_data_point(const std::vector<float> &in, const std::vector<float> &out)
{
    dataset_[ds_n_]->Add(in, out);
    mlp_trigger_redraw_();
}

void mlp_clear()
{
    dataset_[ds_n_]->Clear();
    mlp_trigger_redraw_();
}

void mlp_pretrain_centre_()
{
    if (mlp_stored_output.size() == 0) {
        std::printf("MLP- mlp_stored_output.size() == 0!\n");
        return;
    }
    std::vector<std::vector<float>> features {
        {0.5f, 0.5f, 0.5f, 1.}  // with bias
    };
    std::vector<std::vector<float>> labels {
        mlp_stored_output
    };
    MLP<float>::training_pair_t dataset(features, labels);

    // Re-init weights
    mlp_[nn_n_]->DrawWeights();
    // Train with one point at centre
    mlp_[nn_n_]->Train(dataset,
              1.,
              1000,
              0.00001,
              false);
}

void mlp_set_speed(float speed)
{
    std::printf("MLP- Speed: %f\n", speed);
    speed_ = speed;
}

void mlp_set_expl_mode(te_expl_mode mode)
{
    expl_mode_internal_ = mode;
    redraw_weights_ = true;
    zoom_mode_centre_ = kZoom_mode_reset;
}

void mlp_set_model_idx(size_t idx)
{
    if (idx >= kMaxModels) {
        std::printf("MLP- Model index %u not valid.\n", idx);
    } else {
        std::printf("MLP- Model index: %u.\n", idx);
        nn_n_ = idx;
        mlp_stored_output.clear();
        mlp_stored_weights_ = mlp_[nn_n_]->GetWeights();
        mlp_trigger_redraw_();
    }
}

void mlp_set_dataset_idx(size_t idx)
{
    if (idx >= kMaxDatasets) {
        std::printf("MLP- Dataset index %u not valid.\n", idx);
    } else {
        std::printf("MLP- Dataset index: %u.\n", idx);
        ds_n_ = idx;
    }
}

void mlp_inference_nochannel(ts_joystick_read joystick_read) {

    // Function to zoom and offset by given range
    static const auto zoom_in_ = [](float x, float move_by) {
        float local_range = speed_;
        float max_radius = (0.5f*speed_ + move_by) - 1.0f;
        if (max_radius > 0) {
            local_range -= max_radius;
        }
        float min_radius = -0.5f*speed_ + move_by;
        if (min_radius < 0) {
            local_range += min_radius;
        }
        // Scale and move
        float y = (x - 0.5f) * local_range + move_by;
        xassert(y >= 0 && "MLP- joystick scaling out of range");
        xassert(y <= 1.f && "MLP- joystick scaling out of range");
        return y;
    };

    // If we're zooming in, we want speed to shrink our view
    if (flag_zoom_in_) {
        joystick_read.potX = zoom_in_(joystick_read.potX, zoom_mode_centre_.potX);
        joystick_read.potY = zoom_in_(joystick_read.potY, zoom_mode_centre_.potY);
        joystick_read.potRotate = zoom_in_(joystick_read.potRotate, zoom_mode_centre_.potRotate);
    }

        // Store current joystick read
        mlp_stored_input = joystick_read;

    // Instantiate data in/out
    std::vector<num_t> input{
        joystick_read.potX,
        joystick_read.potY,
        joystick_read.potRotate,
        1.f  // bias
    };
    std::vector<num_t> output(n_output_params_);
    // Run model
    mlp_[nn_n_]->GetOutput(input, &output);

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

        mlp_inference_nochannel(*joystick_read);

    }  // while(true)
}
