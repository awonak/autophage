/**
 * autophage_dsp.cpp — Autophage wave folder implementation.
 */

#include "autophage_dsp.h"

#include <cmath>

namespace autophage_dsp {

namespace {
float sample_rate_hz = 48000.f;
ChannelParams params_[2];

FilterMode filter_mode_ = FilterMode::LowPass;
DistortionRouting dist_routing_ = DistortionRouting::PreFilter;
FeedbackRouting fb_routing_ = FeedbackRouting::RawInput;
bool is_muted_ = false;

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

struct Svf {
    float lp = 0.0f;
    float hp = 0.0f;
    float bp = 0.0f;

    float Process(float in, float cutoff_hz, float res, FilterMode mode) {
        float q = 1.0f - res;
        // Ensure q is never 0 to avoid self-oscillation blowup
        if (q < 0.05f) q = 0.05f;

        float f = 2.0f * std::sin(M_PI * cutoff_hz / sample_rate_hz);
        if (f > 0.99f) f = 0.99f;

        lp = lp + f * bp;
        hp = in - lp - q * bp;
        bp = bp + f * hp;

        switch (mode) {
            case FilterMode::LowPass:
                return lp;
            case FilterMode::BandPass:
                return bp;
            case FilterMode::HighPass:
                return hp;
            default:
                return lp;
        }
    }
};

struct DcBlocker {
    float x_prev = 0.0f;
    float y_prev = 0.0f;
    float R = 0.999f;

    void Init(float sample_rate, float cutoff_hz = 10.0f) {
        R = 1.0f - (M_PI * 2.0f * cutoff_hz / sample_rate);
    }

    float Process(float x) {
        float y = x - x_prev + R * y_prev;
        x_prev = x;
        y_prev = y;
        return y;
    }
};

struct BazzFuss {
    float y_prev = 0.0f;

    float Process(float in, float drive, DcBlocker& dc_block) {
        float gain = 1.0f + drive * 20.0f;
        float k = drive * 0.99f;  // Max feedback is 0.99 for stability

        float x = in * gain + y_prev * k;

        float out;
        if (x > 0.0f) {
            out = 1.0f - std::exp(-x);
        } else {
            out = -0.8f * std::tanh(-x * 1.5f);
        }
        y_prev = out;

        return dc_block.Process(out);
    }
};

struct ChannelState {
    Smoother fold;
    Smoother offset;
    Smoother symmetry;
    Smoother feedback;
    Smoother distortion;
    Smoother filter_cutoff;
    Smoother filter_res;

    Svf filter;
    DcBlocker dist_dc_block;
    BazzFuss distortion_fx;
    float fb_delay = 0.0f;

    void Init(float sample_rate) {
        fold.Init(sample_rate);
        offset.Init(sample_rate);
        symmetry.Init(sample_rate);
        feedback.Init(sample_rate);
        distortion.Init(sample_rate);
        filter_cutoff.Init(sample_rate);
        filter_res.Init(sample_rate);
        dist_dc_block.Init(sample_rate);
    }
};

ChannelState state_[2];

}  // namespace

void SetFilterMode(FilterMode mode) { filter_mode_ = mode; }
FilterMode GetFilterMode() { return filter_mode_; }

void SetMuted(bool muted) { is_muted_ = muted; }
bool GetMuted() { return is_muted_; }

void SetDistortionRouting(DistortionRouting routing) { dist_routing_ = routing; }
DistortionRouting GetDistortionRouting() { return dist_routing_; }

void SetFeedbackRouting(FeedbackRouting routing) { fb_routing_ = routing; }
FeedbackRouting GetFeedbackRouting() { return fb_routing_; }

void Init(float sample_rate) {
    sample_rate_hz = sample_rate;
    params_[0] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16000.0f, 0.0f};
    params_[1] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 16000.0f, 0.0f};
    state_[0].Init(sample_rate);
    state_[1].Init(sample_rate);
}

void SetChannel(uint8_t ch, const ChannelParams& p) {
    if (ch > 1) return;
    params_[ch] = p;
}

inline float ProcessFold(float in, float fold_amount, float offset, float symmetry) {
    float gain = 1.0f + fold_amount * 10.0f;
    float dc_offset = offset * symmetry;
    float x = (in + dc_offset) * gain;

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
        for (int ch = 0; ch < 2; ch++) {
            ChannelState& s = state_[ch];
            ChannelParams& p = params_[ch];

            float fold = s.fold.Process(p.fold);
            float offset = s.offset.Process(p.offset);
            float symmetry = s.symmetry.Process(p.symmetry);
            float fb_amt = s.feedback.Process(p.feedback);
            float dist = s.distortion.Process(p.distortion);
            float cutoff = s.filter_cutoff.Process(p.filter_cutoff);
            float res = s.filter_res.Process(p.filter_res);

            // Feedback mixing
            float raw_in = in[ch][i];
            float mixed_in = raw_in + s.fb_delay * fb_amt;

            // Wave folding
            float folded = ProcessFold(mixed_in, fold, offset, symmetry);

            // FX Chain
            float fx_signal = folded;

            if (dist_routing_ == DistortionRouting::PreFilter) {
                fx_signal = s.distortion_fx.Process(fx_signal, dist, s.dist_dc_block);
                fx_signal = s.filter.Process(fx_signal, cutoff, res, filter_mode_);
            } else {
                fx_signal = s.filter.Process(fx_signal, cutoff, res, filter_mode_);
                fx_signal = s.distortion_fx.Process(fx_signal, dist, s.dist_dc_block);
            }

            // Update feedback delay
            if (fb_routing_ == FeedbackRouting::RawInput) {
                s.fb_delay = folded;
            } else {
                s.fb_delay = fx_signal;
            }

            if (is_muted_) {
                out[ch][i] = raw_in;
            } else {
                out[ch][i] = fx_signal;
            }
        }
    }
}

}  // namespace autophage_dsp
