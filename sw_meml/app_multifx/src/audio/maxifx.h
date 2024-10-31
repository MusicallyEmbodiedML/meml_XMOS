#pragma once
#include <math.h>

// class maxiSettings
// {
// public:
//     maxiSettings();

//     /*! The sample rate */
//     static size_t sampleRate;
//     static float one_over_sampleRate;
//     static size_t channels;
//     static size_t bufferSize;
//     /**
//      * Configure Maximilian
//      * \param initSampleRate the sample rate
//      * \param initChannels the number of audio channels
//      * \param initBufferSize the buffer size of your audio system
//      */
//     static void setup(size_t initSampleRate, size_t initChannels, size_t initBufferSize)
//     {
//         maxiSettings::sampleRate = initSampleRate;
//         maxiSettings::one_over_sampleRate = 1.f / static_cast<float>(maxiSettings::sampleRate);
//         maxiSettings::channels = initChannels;
//         maxiSettings::bufferSize = initBufferSize;
//     }
//     static size_t getSampleRate()
//     {
//         return maxiSettings::sampleRate;
//     }
// };


// class maxiDelayline
// {

// public:
//     maxiDelayline() {
//         memset( memory, 0, kDl_max_length * sizeof(float) );        
//     };

//     /*! Apply a delay to a signal \param input a signal, \param size the size of the delay in samples \param feedback the amount of feedback*/
//     float play(float input, size_t size, float feedback);

//     static const unsigned int kDl_max_length = 48000;

//  protected:
//     float frequency;
//     int phase;
//     float startphase;
//     float endphase;
//     float output;
//     float memory[kDl_max_length];
// };

// float maxiDelayline::play(float input, size_t size, float feedback)  {
// 	if (size >= kDl_max_length) {
// 		return 0;
// 	}
// 	if ( phase >=size ) {
// 		phase = 0;
// 	}
// 	output=memory[phase];
// 	memory[phase]=(memory[phase] * feedback) + input;
// 	phase+=1;
// 	return(output);

// }

// class  maxiOsc
// {
// private:
//     float phase;
//     float constant_by_one_over_sr_;

// public:
//     maxiOsc();
//     void UpdateParams(void);
//     /*!Triangle oscillator \param frequency in Hz */
//     float triangle(float frequency);
//     /*!Set the phase of the oscillator \param phaseIn The phase, from 0 to 1*/
//     void phaseReset(float phaseIn);
// };

// maxiOsc::maxiOsc(){
// 	phase = 0.0;
// }

// void maxiOsc::phaseReset(float phaseIn) {
// 	//This allows you to set the phase of the oscillator to anything you like.
// 	phase=phaseIn;
// }

// float maxiOsc::triangle(float frequency) {
//     float output;
// 	if ( phase >= 1.0f ) phase -= 1.0;
// 	phase += maxiSettings::one_over_sampleRate * frequency;
// 	if (phase <= 0.5f ) {
// 		output =(phase - 0.25f) * 4.f;
// 	} else {
// 		output =((1.0f-phase) - 0.25f) * 4.f;
// 	}
// 	return output;

// }


// class maxiFlanger
// {
// public:
//     //delay = delay time - ~800 sounds good
//     //feedback = 0 - 1
//     //speed = lfo speed in Hz, 0.0001 - 10 sounds good
//     //depth = 0 - 1
//     /**
//      * Apply a flanger effect to a signal
//      * \param input a signal
//      * \param delay the amount of delay (in milliseconds, recommended 1-1000)
//      * \param feedback the amount of feedback, 0-1
//      * \param speed the speed of the flanger LFO, in Hz
//      * \param depth the depth of the LFO, 0-1
//      * \returns the input signal, flanged
//      */

//     float flange(const float input, const unsigned int delay, const float feedback, const float speed, const float depth);
//     maxiDelayline dl;
//     maxiOsc lfo;
// };

// inline float maxiFlanger::flange(const float input, const unsigned int delay, const float feedback, const float speed, const float depth)
// {
//     float output;
//     float lfoVal = lfo.triangle(speed);
//     output = dl.dl(input, delay + (lfoVal * depth * delay) + 1, feedback);
//     float normalise = (1 - fabs(output));
//     output *= normalise;
//     return (output + input) / 2.0;
// }


// class maxiNonlinearity
// {
// public:
//     maxiNonlinearity(){};
//     /** atan distortion, see http://www.musicdsp.org/showArchiveComment.php?ArchiveID=104
//     * \param in A signal
//     * \param shape from 1 (soft clipping) to infinity (hard clipping)
//     */ 
//     float atanDist(const float in, const float shape);
//     /** Faster but 'lower quality' version of atan distortion
//      * \param in A signal
//      * \param shape from 1 (soft clipping) to infinity (hard clipping)
//     */
//     float fastAtanDist(const float in, const float shape);
//     /** Cliping with nicer harmonics \param x A signal*/
//     float softclip(float x);
//     /** Cliping with nastier harmonics \param x A signal*/
//     float hardclip(float x);
//     /**
//      * asymmetric clipping: chose the shape of curves for both positive and negative values of x
//      * try it here https://www.desmos.com/calculator/to6eixatsa
//      * \param x A signal
//      * \param a Exponent for the positive curve
//      * \param b Exponent for the negative curve
//      */
//     float asymclip(float x, float a, float b);
//     /*! Fast atan distortion \param x A signal */
//     float fastatan(float x);
// };

// inline float maxiNonlinearity::asymclip(float x, float a, float b)
// {

//     if (x >= 1)
//     {
//         x = 1;
//     }
//     else if (x <= -1)
//     {
//         x = -1;
//     }
//     else if (x < 0)
//     {
//         x = -(std::powf(-x, a));
//     }
//     else
//     {
//         x = std::powf(x, b);
//     }
//     return x;
// }

// inline float maxiNonlinearity::hardclip(float x)
// {
//     x = x >= 1 ? 1 : (x <= -1 ? -1 : x);
//     return x;
// }

// inline float maxiNonlinearity::softclip(float x)
// {
//     if (x >= 1.f)
//     {
//         x = 1.f;
//     }
//     else if (x <= -1.f)
//     {
//         x = -1.f;
//     }
//     else
//     {
//         x = (2.f / 3.0f) * (x - std::powf(x, 3.f) / 3.0f);
//     }
//     return x;
// }

// inline float maxiNonlinearity::fastatan(float x)
// {
//     return (x / (1.0f + 0.28f * (x * x)));
// }

// inline float maxiNonlinearity::atanDist(const float in, const float shape)
// {
//     float out;
//     out = (1.0f / atan(shape)) * atan(in * shape);
//     return out;
// }

// inline float maxiNonlinearity::fastAtanDist(const float in, const float shape)
// {
//     float out;
//     out = (1.0f / fastatan(shape)) * fastatan(in * shape);
//     return out;
// }
