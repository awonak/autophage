/**
 * autophage_dsp.cpp — Autophage wave folder implementation.
 */

#include "autophage_dsp.h"

namespace autophage_dsp {

namespace {
    float sample_rate_hz = 48000.f;
    ChannelParams params_[2];
} // namespace

void Init(float sample_rate) { 
    sample_rate_hz = sample_rate; 
    params_[0] = {0.0f, 0.0f, 0.0f};
    params_[1] = {0.0f, 0.0f, 0.0f};
}

void SetChannel(uint8_t ch, const ChannelParams& p)
{
    if (ch > 1) return;
    params_[ch] = p;
}

void Process(const float* const* in,
             float**             out,
             size_t              n)
{
    for (size_t i = 0; i < n; i++)
    {
        // Simple passthrough for now
        out[0][i] = in[0][i];
        out[1][i] = in[1][i];
    }
}

} // namespace autophage_dsp
