#include "mlp_task.hpp"
#include "MLP.h"

#include <cstdio>

#define MLP_STANDALONE    1
#if MLP_STANDALONE
using ts_joystick_read = float[3];
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
static MLP<float> *mlp_;
static char mlp_mem_[sizeof(MLP<float>)];


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

void mlp_task()
{
    while (1) {
        
    }
}
