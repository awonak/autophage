/**
 * autophage_dsp.h — Autophage wave folder DSP interface.
 */

#pragma once

#include <cstddef>
#include <cstdint>


namespace autophage_dsp {

/** Per-channel parameters for wave folding. */
struct ChannelParams
{
    float fold;      // Amount of wave folding
    float offset;    // DC offset
    float symmetry;  // Symmetry/mix of offset
};

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
