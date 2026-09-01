/**
 * autophage_dsp.cpp — Autophage wave folder implementation.
 */

#include "autophage_dsp.h"

#include <cmath>

namespace autophage_dsp {

namespace {
float sample_rate_hz = 48000.f;
ChannelParams params_[2];

DistortionRouting dist_routing_ = DistortionRouting::PreFilter;
FeedbackRouting fb_routing_ = FeedbackRouting::PostFx;
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

struct DjFilter {
    float s1 = 0.0f;
    float s2 = 0.0f;

    void Reset() {
        s1 = 0.0f;
        s2 = 0.0f;
    }

    float Process(float in, float filter_val, float q_val) {
        // filter_val in [-1.0, 1.0]
        float abs_val = std::abs(filter_val);
        if (abs_val < 0.001f) {
            s1 = 0.0f;
            s2 = 0.0f;
            return in;
        }

        float t = abs_val; // 0.0 -> 1.0
        float cutoff_hz;
        if (filter_val < 0.0f) {
            // LowPass sweep: 20000 Hz (near 0) down to 30 Hz (at -1.0)
            cutoff_hz = 20000.0f * std::pow(30.0f / 20000.0f, t);
        } else {
            // HighPass sweep: 20 Hz (near 0) up to 16000 Hz (at +1.0)
            cutoff_hz = 20.0f * std::pow(16000.0f / 20.0f, t);
        }

        float nyquist = sample_rate_hz * 0.49f;
        if (cutoff_hz > nyquist) cutoff_hz = nyquist;
        if (cutoff_hz < 10.0f)   cutoff_hz = 10.0f;

        // Resonance: q_val = 0.0 is Butterworth (k = 1.414), 1.0 is resonant (k = 0.08)
        float k_target = 1.414f - q_val * 1.334f;
        if (k_target < 0.08f) k_target = 0.08f;

        // Taper resonance near center to prevent low-end resonance bumps near 12 o'clock
        float q_blend = (t < 0.08f) ? (t / 0.08f) : 1.0f;
        float k = 1.414f + (k_target - 1.414f) * q_blend;

        // 2-pole TPT (Topology-Preserving Transform) SVF
        float g = std::tan(static_cast<float>(M_PI) * cutoff_hz / sample_rate_hz);
        float denom = 1.0f + g * (g + k);

        float hp = (in - (k + g) * s1 - s2) / denom;
        float v1 = g * hp;
        float bp = v1 + s1;
        s1 = 2.0f * v1 + s1;
        float v2 = g * bp;
        float lp = v2 + s2;
        s2 = 2.0f * v2 + s2;

        // Flush denormals
        if (std::abs(s1) < 1e-15f) s1 = 0.0f;
        if (std::abs(s2) < 1e-15f) s2 = 0.0f;

        float filtered = (filter_val < 0.0f) ? lp : hp;

        // Smooth wet/dry crossfade across center transition
        if (t < 0.03f) {
            float blend = t / 0.03f;
            return in + blend * (filtered - in);
        }

        return filtered;
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
        float read_pos = static_cast<float>(write_pos) - delay_samples;
        if (read_pos < 0.0f) read_pos += 4096.0f;
        uint32_t index_int = static_cast<uint32_t>(read_pos);
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

struct OnePoleLpf {
    float val = 0.0f;
    float coeff = 0.15f;

    void Init(float sample_rate, float cutoff_hz = 1800.0f) {
        coeff = 1.0f - std::exp(-2.0f * static_cast<float>(M_PI) * cutoff_hz / sample_rate);
    }

    float Process(float in) {
        val += coeff * (in - val);
        return val;
    }
};

struct ChannelState {
    Smoother fold;
    Smoother symmetry;
    Smoother warp;
    Smoother feedback;
    Smoother distortion;
    Smoother filter;
    Smoother filter_q;

    DjFilter filter_fx;
    DcBlocker dist_dc_block;
    DcBlocker fb_dc_block;
    OnePoleLpf fb_damp_filter;
    BazzFuss distortion_fx;
    DelayLine fb_delay;

    void Init(float sample_rate) {
        fold.Init(sample_rate);
        symmetry.Init(sample_rate);
        warp.Init(sample_rate);
        feedback.Init(sample_rate);
        distortion.Init(sample_rate);
        filter.Init(sample_rate);
        filter_q.Init(sample_rate);
        dist_dc_block.Init(sample_rate);
        fb_dc_block.Init(sample_rate, 20.0f);
        fb_damp_filter.Init(sample_rate, 1800.0f);
    }
};

ChannelState state_[2];

}  // namespace

void SetInputMode(InputMode mode) { input_mode_ = mode; }
InputMode GetInputMode() { return input_mode_; }

void SetBypassed(bool bypassed) { is_bypassed_ = bypassed; }
bool GetBypassed() { return is_bypassed_; }

void SetDistortionRouting(DistortionRouting routing) { dist_routing_ = routing; }
DistortionRouting GetDistortionRouting() { return dist_routing_; }

void SetFeedbackRouting(FeedbackRouting routing) { fb_routing_ = routing; }
FeedbackRouting GetFeedbackRouting() { return fb_routing_; }

void Init(float sample_rate) {
    sample_rate_hz = sample_rate;
    params_[0] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    params_[1] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    state_[0].Init(sample_rate);
    state_[1].Init(sample_rate);
}

void SetChannel(uint8_t ch, const ChannelParams& p) {
    if (ch > 1) return;
    params_[ch] = p;
}

inline float ProcessFold(float in, float fold_amount, float symmetry, float warp) {
    // 1. Non-linear Wave Warp (slope & curvature distortion)
    float x = in;
    if (warp != 0.0f) {
        x = x + warp * (x * x * x - x);
    }

    // 2. Calculate overall gain
    float gain = 1.0f + fold_amount * 10.0f;
    x *= gain;

    // Sanity check for bad floating point values
    if (std::isnan(x) || std::isinf(x)) return 0.0f;

    // 3. Apply DC offset symmetry bias (shifts waveform center before folding)
    x += symmetry * 2.0f;
    x = std::tanh(x);

    // Re-scale back to fold range after soft saturation
    x *= gain;

    // 4. Safety Clamps
    if (x > 100.0f) x = 100.0f;
    if (x < -100.0f) x = -100.0f;

    // 5. Piecewise Linear Triangle Folding Loop
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
    const float kFbDelaySamples = 72.0f;  // ~1.5ms analog-style delay tap
    for (size_t i = 0; i < n; i++) {
        for (int ch = 0; ch < 2; ch++) {
            ChannelState& s = state_[ch];
            ChannelParams& p = params_[ch];

            float fold = s.fold.Process(p.fold);
            float symmetry = s.symmetry.Process(p.symmetry);
            float warp_val = s.warp.Process(p.warp);
            float fb_amt = s.feedback.Process(p.feedback);
            float dist = s.distortion.Process(p.distortion);
            float flt_val = s.filter.Process(p.filter);
            float flt_q = s.filter_q.Process(p.filter_q);

            // Input Routing
            float raw_in = in[ch][i];
            if (input_mode_ == InputMode::StereoLink && ch == 1) {
                raw_in = in[0][i];
            }

            // Feedback mixing with damping, DC blocking, and soft saturation
            float fb_raw = s.fb_delay.Read(kFbDelaySamples);
            float fb_damped = s.fb_damp_filter.Process(fb_raw);
            float fb_conditioned = s.fb_dc_block.Process(fb_damped);
            float fb_clipped = std::tanh(fb_conditioned * 1.2f);
            float fb_gain = fb_amt * fb_amt * 1.2f;
            float mixed_in = raw_in + fb_clipped * fb_gain;

            // Wave folding with Warp slope distortion
            float folded = ProcessFold(mixed_in, fold, symmetry, warp_val);

            // FX Chain
            float fx_signal = folded;

            if (dist_routing_ == DistortionRouting::PreFilter) {
                fx_signal = s.distortion_fx.Process(fx_signal, dist, s.dist_dc_block);
                fx_signal = s.filter_fx.Process(fx_signal, flt_val, flt_q);
            } else {
                fx_signal = s.filter_fx.Process(fx_signal, flt_val, flt_q);
                fx_signal = s.distortion_fx.Process(fx_signal, dist, s.dist_dc_block);
            }

            // Update feedback delay
            if (fb_routing_ == FeedbackRouting::RawInput) {
                s.fb_delay.Write(folded);
            } else {
                s.fb_delay.Write(fx_signal);
            }

            if (is_bypassed_) {
                out[ch][i] = raw_in;
            } else {
                out[ch][i] = fx_signal;
            }
        }
    }
}

}  // namespace autophage_dsp
