/**
 * autophage_dsp.h — Autophage wave folder DSP interface.
 */

#pragma once

#include <cstddef>
#include <cstdint>


namespace autophage_dsp {

enum class InputMode {
    Normal = 0,
    StereoLink,
    NumModes
};

enum class FilterMode {
    LowPass = 0,
    BandPass,
    HighPass,
    NumModes
};

enum class DistortionRouting {
    Bypass = 0,
    PreFilter,
    PostFilter,
    NumModes
};



/** Per-channel parameters for wave folding and effects. */
struct ChannelParams
{
    float fold;           // Amount of wave folding (0..1)
    float offset;         // DC offset (-1..1)
    float symmetry;       // Symmetry/mix of offset (0..1)
    float feedback;       // Feedback amount (0..1)
    float feedback_time;  // Feedback delay time (in seconds, e.g. 0.001 to 0.050)
    float distortion;     // Distortion amount (0..1)
    float distortion_bias;// Distortion DC bias (-1..1)
    float filter_cutoff;  // Filter cutoff in Hz
    float filter_res;     // Filter resonance (0..1)
};

/** Global routing parameters */
void SetInputMode(InputMode mode);
InputMode GetInputMode();

void SetFilterMode(FilterMode mode);
FilterMode GetFilterMode();

void SetDistortionRouting(DistortionRouting routing);
DistortionRouting GetDistortionRouting();



void SetBypassed(bool bypassed);
bool GetBypassed();

/** Cache the sample rate. Call once after hw.Init(). */
void Init(float sample_rate);

/** Update DSP parameters for one channel (0 = left, 1 = right). */
void SetChannel(uint8_t ch, const ChannelParams& p);

/**
 * Audio callback processing block.
 * Uses float** to ensure hardware independence for local testing.
 */
void Process(const float* const* in,
             float**             out,
             size_t              n);

} // namespace autophage_dsp
