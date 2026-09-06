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

enum class DistortionRouting {
    PreFilter = 0,
    PostFilter,
    NumModes
};

enum class FeedbackRouting {
    RawInput = 0,
    PostFx
};

/** Per-channel parameters for wave folding and effects. */
struct ChannelParams
{
    float fold;           // Amount of wave folding (0..1)
    float symmetry;       // Asymmetrical drive (-1..1)
    float warp;           // Slope & curvature distortion (-1..1)
    float feedback;       // Feedback amount (0..1)
    float distortion;     // Distortion amount (0..1)
    float filter;         // DJ filter amount (-1..1, CCW=LP, Center=Flat, CW=HP)
    float filter_q;       // Filter resonance / Q (0..1)
};

/** Global routing parameters */
void SetInputMode(InputMode mode);
InputMode GetInputMode();

void SetDistortionRouting(DistortionRouting routing);
DistortionRouting GetDistortionRouting();

void SetFeedbackRouting(FeedbackRouting routing);
FeedbackRouting GetFeedbackRouting();

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
