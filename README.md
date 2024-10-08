# meml_XMOS
Minimal repository for MEML apps/PoC's on the XMOS xcore.ai platform

## Application
The meml application and hardware demonstrates a self-contained, "Tunable ML" instrument. For more background on the project, see the [MEML website](https://users.sussex.ac.uk/~ck84/meml/). 

The application implements the following components:
- Peripherals: I2C, I2S (pulled from the XCommon CMake build system), UART modified to compile as c++ from <here>
- [Machine Learning](https://github.com/MusicallyEmbodiedML/MLP_XMOS): A Multi-Layer Perceptron (MLP) implementation forked from a [simple implementation](https://github.com/davidalbertonogueira/MLP) written in c++ and modified to run on the XMOS architecture 
- [FM synthesis](https://github.com/MusicallyEmbodiedML/fmsynth): A basic implementation written in c++

## Hardware
This device, in its prototype stage, can be built using the following components:

- [XK-EVK-XU316 xcore.ai Evaluation Kit](https://www.xmos.com/xk-evk-xu316)
- Joystick (model link)
- switches (model link)

### Hardware design

- Insert hardware diagram here

## Software build system
This software is compiles using the XCommon CMake build system provided by XMOS.

This build system is included in the installation of the [XMOS XTC Tools ](https://www.xmos.com/software-tools/) from version 15.3.0 onwards.

[Documentation](https://www.xmos.com/documentation/XM-014363-PC/html/intro/index.html) for the XTC Tools provides installation instructions, including how to set up your Evaluation Kit (or XTAG programmer, if you are using one separately) so that your user has access rights to connect to it.

[Documentation](https://www.xmos.com/documentation/XM-015090-PC/html/doc/introduction.html) for the XCommon CMake build system provides details of other requirements (CMake and git versions), as well as an overview of the build system and the custom file/dependency structure it requires, should you wish to make modifications or extensions of this application in a fork.

N.B. on linux, we have found that installing `libtinfo5` and `libncurses5` has been necessary for the command-line output of the build system to work.
