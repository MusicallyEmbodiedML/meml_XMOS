# meml_XMOS
Minimal repository for MEML apps/PoC's on the XMOS xcore.ai platform

## Application
The meml application and hardware demonstrates a self-contained, "Tunable ML" instrument. For more background on the project, see the [MEML website](https://users.sussex.ac.uk/~ck84/meml/). 

The application implements the following components:
Peripherals: 
- I2C, code pulled using the Xcommon CMake build system (see below)
- I2S, code modified from examples in [XMOS' `lib_board_support` library](https://github.com/xmos/lib_board_support) 
- UART, code copied directly from the [XMOS `fwk_io library`](https://github.com/xmos/fwk_io), which is a module that supports the [XMOS `xcore_iot` repo](https://github.com/xmos/xcore_iot).  
- [Machine Learning](https://github.com/MusicallyEmbodiedML/MLP_XMOS): A Multi-Layer Perceptron (MLP) implementation forked from a [simple implementation](https://github.com/davidalbertonogueira/MLP) written in c++ and modified to run on the XMOS architecture 
- [FM synthesis](https://github.com/MusicallyEmbodiedML/fmsynth): A basic implementation copied and modified from  the [Maximilian c++ synth library](https://github.com/micknoise/Maximilian).

## Hardware
This device, in its prototype stage, can be built using the following components:

- [XK-EVK-XU316 xcore.ai Evaluation Kit](https://www.xmos.com/xk-evk-xu316)
- [Raspberry Pi Pico](https://www.raspberrypi.com/documentation/microcontrollers/pico-series.html)
- A 3-axis joystick, model R300B-M2
- push button switches
- A Raspberry Pi Pico acting as a hardware interface to the joystick and push buttons ([firmware](https://github.com/MusicallyEmbodiedML/pico-interface-fmsynth))

### Hardware design

- A design and electronics diagram to follow

## Getting started

Since this repo currently makes use of git `submodules`, you must run the following after cloning the repo:

```
git submodule update --init --recursive
```

You must additionally run the following to keep the submodules up to date with their latest changes:
```
git submodule update --recursive --remote
```

The `lib_meml`, `lib_synth` and `lib_uart` modules must be compiled as static libraries before compiling the full project. Documentation to achieve this is linked in the `Build system` section of this readme.

## Build system
This software is compiles using the XCommon CMake build system provided by XMOS.

This build system is included in the installation of the [XMOS XTC Tools ](https://www.xmos.com/software-tools/) from version 15.3.0 onwards.

[Documentation](https://www.xmos.com/documentation/XM-014363-PC/html/intro/index.html) for the XTC Tools provides installation instructions, including how to set up your Evaluation Kit (or XTAG programmer, if you are using one separately) so that your user has access rights to connect to it.

[Documentation](https://www.xmos.com/documentation/XM-015090-PC/html/doc/introduction.html) for the XCommon CMake build system provides details of other requirements (CMake and git versions), as well as an overview of the build system and the custom file/dependency structure it requires, should you wish to make modifications or extensions of this application in a fork.

N.B. on linux, we have found that installing `libtinfo5` and `libncurses5` has been necessary for the command-line output of the build system to work.
