/**
 * autophage_dsp.cpp — Autophage wave folder implementation.
 */

#include "autophage_dsp.h"

#include <cmath>

namespace autophage_dsp {

namespace {
float sample_rate_hz = 48000.f;
ChannelParams params_[2];

struct Smoother {
    float val = 0.0f;
    float coeff = 0.01f;

    void Init(float sample_rate, float time_ms = 20.0f) {
        coeff = 1.0f - std::exp(-1.0f / (sample_rate * (time_ms / 1000.0f)));
    }
    
    float Process(float target) {
        val += coeff * (target - val);
        return val;
    }
};

struct ChannelSmoothers {
    Smoother fold;
    Smoother offset;
    Smoother symmetry;

    void Init(float sample_rate) {
        fold.Init(sample_rate);
        offset.Init(sample_rate);
        symmetry.Init(sample_rate);
    }
};

ChannelSmoothers smoothers_[2];

}  // namespace

void Init(float sample_rate) {
    sample_rate_hz = sample_rate;
    params_[0] = {0.0f, 0.0f, 0.0f};
    params_[1] = {0.0f, 0.0f, 0.0f};
    smoothers_[0].Init(sample_rate);
    smoothers_[1].Init(sample_rate);
}

void SetChannel(uint8_t ch, const ChannelParams& p) {
    if (ch > 1) return;
    params_[ch] = p;
}

inline float ProcessFold(float in, float fold_amount, float offset, float symmetry) {
    float gain = 1.0f + fold_amount * 10.0f;
    float dc_offset = offset * symmetry;
    // Inject the DC offset before applying gain to ensure the signal is 
    // pushed deeply into the folding thresholds, making the negative folding 
    // (and positive folding) highly responsive to the offset knob.
    float x = (in + dc_offset) * gain;

    // Prevent infinite loop on bad input
    if (std::isnan(x) || std::isinf(x)) return 0.0f;
    if (x > 100.0f) x = 100.0f;
    if (x < -100.0f) x = -100.0f;

    int max_folds = 100;
    while ((x > 1.0f || x < -1.0f) && max_folds-- > 0) {
        if (x > 1.0f)
            x = 2.0f - x;
        else if (x < -1.0f)
            x = -2.0f - x;
    }
    return x;
}

void Process(const float* const* in,
             float** out,
             size_t n) {
    for (size_t i = 0; i < n; i++) {
        float fold0 = smoothers_[0].fold.Process(params_[0].fold);
        float offset0 = smoothers_[0].offset.Process(params_[0].offset);
        float symmetry0 = smoothers_[0].symmetry.Process(params_[0].symmetry);
        out[0][i] = ProcessFold(in[0][i], fold0, offset0, symmetry0);

        float fold1 = smoothers_[1].fold.Process(params_[1].fold);
        float offset1 = smoothers_[1].offset.Process(params_[1].offset);
        float symmetry1 = smoothers_[1].symmetry.Process(params_[1].symmetry);
        out[1][i] = ProcessFold(in[1][i], fold1, offset1, symmetry1);
    }
}

}  // namespace autophage_dsp
