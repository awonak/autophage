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
DistortionRouting dist_routing_ = DistortionRouting::Bypass;
bool is_bypassed_ = false;
InputMode input_mode_ = InputMode::Normal;

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

struct DelayLine {
    float buf[4096] = {0.0f};
    uint32_t write_pos = 0;

    float Read(float delay_samples) {
        // Clamp to valid buffer range
        if (delay_samples < 1.0f) delay_samples = 1.0f;
        if (delay_samples > 4094.0f) delay_samples = 4094.0f;
        float read_pos = static_cast<float>(write_pos) - delay_samples;
        // Use fmod to handle wraparound correctly regardless of magnitude
        if (read_pos < 0.0f) read_pos = std::fmod(read_pos, 4096.0f) + 4096.0f;
        uint32_t index_int = static_cast<uint32_t>(read_pos) & 4095;
        float frac = read_pos - static_cast<float>(index_int);
        float a = buf[index_int];
        float b = buf[(index_int + 1) & 4095];
        return a + frac * (b - a);
    }

    void Write(float sample) {
        buf[write_pos] = sample;
        write_pos = (write_pos + 1) & 4095;
    }
};

struct BazzFuss {
    // Models a Darlington-pair BJT + collector-base Germanium diode (Hemmo Bazz Fuss)
    // Positive swing: transistor saturation + Germanium diode soft knee
    // Negative swing: transistor cutoff (harder, more abrupt than saturation side)
    float Process(float in, float drive, float bias, DcBlocker& dc_block) {
        // Darlington pair: significantly higher gain than single BJT
        float gain = 2.0f + drive * 50.0f;

        // Bias shifts the transistor's operating point
        float x = in * gain + bias * 0.5f;

        float out;
        if (x >= 0.0f) {
            // Transistor saturation path
            float sat = 1.0f - std::exp(-x);
            // Germanium diode clamp (~0.3V forward voltage, rounder/softer than silicon)
            constexpr float kDiodeThreshold = 0.3f;
            constexpr float kDiodeKnee = 0.2f;
            if (sat > kDiodeThreshold) {
                sat = kDiodeThreshold + kDiodeKnee * std::tanh((sat - kDiodeThreshold) / kDiodeKnee);
            }
            out = sat;
        } else {
            // Transistor cutoff path: harder clip, models base-emitter reverse bias
            out = -0.95f * (1.0f - std::exp(x * 0.8f));
        }

        return dc_block.Process(out);
    }
};

struct ChannelState {
    Smoother fold;
    Smoother offset;
    Smoother symmetry;
    Smoother feedback;
    Smoother feedback_time;
    Smoother distortion;
    Smoother distortion_bias;
    Smoother filter_cutoff;
    Smoother filter_res;

    Svf filter;
    DcBlocker dist_dc_block;
    BazzFuss distortion_fx;
    DelayLine fb_delay;
    // One-pole lowpass on the feedback tap — removes harsh transients
    // before the signal re-enters the fold, mimicking analog RC capacitance
    Smoother fb_filter;

    void Init(float sample_rate) {
        fold.Init(sample_rate);
        offset.Init(sample_rate);
        symmetry.Init(sample_rate);
        feedback.Init(sample_rate);
        feedback_time.Init(sample_rate);
        distortion.Init(sample_rate);
        distortion_bias.Init(sample_rate);
        filter_cutoff.Init(sample_rate);
        filter_res.Init(sample_rate);
        dist_dc_block.Init(sample_rate);
        // ~3 ms time constant: smooths the feedback without killing its character
        fb_filter.Init(sample_rate, 3.0f);
    }
};

ChannelState state_[2];

}  // namespace

void SetInputMode(InputMode mode) { input_mode_ = mode; }
InputMode GetInputMode() { return input_mode_; }

void SetFilterMode(FilterMode mode) { filter_mode_ = mode; }
FilterMode GetFilterMode() { return filter_mode_; }

void SetBypassed(bool bypassed) { is_bypassed_ = bypassed; }
bool GetBypassed() { return is_bypassed_; }

void SetDistortionRouting(DistortionRouting routing) { dist_routing_ = routing; }
DistortionRouting GetDistortionRouting() { return dist_routing_; }

void Init(float sample_rate) {
    sample_rate_hz = sample_rate;
    params_[0] = {0.0f, 0.0f, 0.0f, 0.0f, 0.001f, 0.0f, 0.0f, 16000.0f, 0.0f};
    params_[1] = {0.0f, 0.0f, 0.0f, 0.0f, 0.001f, 0.0f, 0.0f, 16000.0f, 0.0f};
    state_[0].Init(sample_rate);
    state_[1].Init(sample_rate);
}

void SetChannel(uint8_t ch, const ChannelParams& p) {
    if (ch > 1) return;
    params_[ch] = p;
}

inline float ProcessFold(float in, float fold_amount, float offset, float symmetry) {
    float gain = 1.0f + fold_amount * 100.0f;
    float dc_offset = offset * symmetry;
    float x = (in + dc_offset) * gain;

    if (std::isnan(x) || std::isinf(x)) return 0.0f;
    if (x > 250.0f) x = 250.0f;
    if (x < -250.0f) x = -250.0f;

    int max_folds = 250;
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
            float fb_time = s.feedback_time.Process(p.feedback_time);
            float dist = s.distortion.Process(p.distortion);
            float dist_bias = s.distortion_bias.Process(p.distortion_bias);
            float cutoff = s.filter_cutoff.Process(p.filter_cutoff);
            float res = s.filter_res.Process(p.filter_res);

            // Input Routing
            float raw_in = in[ch][i];
            if (input_mode_ == InputMode::StereoLink && ch == 1) {
                raw_in = in[0][i];
            }

            // Feedback mixing
            // (B) Lowpass-filter the feedback tap to remove harsh high-freq transients
            float delay_samples = fb_time * sample_rate_hz;
            float fb_raw = s.fb_delay.Read(delay_samples);
            float fb_filtered = s.fb_filter.Process(fb_raw);
            // (C) Quadratic scale: slower approach to instability, more usable range
            float fb_scaled = fb_amt * fb_amt;
            // (A) Soft-clamp the mixed signal before the fold — prevents gain runaway
            float mixed_in = std::tanh(raw_in + fb_filtered * fb_scaled);

            // Wave folding
            float folded = ProcessFold(mixed_in, fold, offset, symmetry);

            // FX Chain
            float fx_signal = folded;

            if (dist_routing_ == DistortionRouting::Bypass) {
                fx_signal = s.filter.Process(fx_signal, cutoff, res, filter_mode_);
            } else if (dist_routing_ == DistortionRouting::PreFilter) {
                fx_signal = s.distortion_fx.Process(fx_signal, dist, dist_bias, s.dist_dc_block);
                fx_signal = s.filter.Process(fx_signal, cutoff, res, filter_mode_);
            } else {
                fx_signal = s.filter.Process(fx_signal, cutoff, res, filter_mode_);
                fx_signal = s.distortion_fx.Process(fx_signal, dist, dist_bias, s.dist_dc_block);
            }

            // Always feed back the post-FX signal
            s.fb_delay.Write(fx_signal);

            if (is_bypassed_) {
                out[ch][i] = raw_in;
            } else {
                out[ch][i] = fx_signal;
            }
        }
    }
}

}  // namespace autophage_dsp
