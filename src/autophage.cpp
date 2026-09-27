/**
 * autophage.cpp — Alchemy Lab dual mono wave folder.
 */ \
#include "alchemy/host_link/host.h"
#include "alchemy/hw/alchemy_lab.h"
#include "alchemy/hw/alchemy_lab_v2_layout.h"
#include "alchemy/surface/button_bank.h"
#include "alchemy/surface/control_loop.h"
#include "alchemy/surface/cv_matrix.h"
#include "alchemy/surface/manual.h"
#include "alchemy/surface/page.h"
#include "alchemy/surface/pager.h"
#include "alchemy/surface/param_lock.h"
#include "alchemy/surface/presets.h"
#include "alchemy/surface/settings.h"
#include "alchemy/surface/virtual_button.h"
#include "alchemy/surface/virtual_knob.h"
#include "autophage_dsp.h"
#include "autophage_palette.h"
#include "daisy_seed.h"

using namespace alchemy;
using namespace autophage::palette;

enum : uint8_t {
    kPageFold = 0,
    kPageDestroy = 1,
    kPageQ = 2,
    kNumPages = 3,
};

/* Page 1: Left Channel Wave Folder */
static VirtualKnob l_fold =
    VirtualKnob(kPotTopLeft, "Fold 1")
        .Linear(0.0f, 1.0f)
        .Ring(Level(kFold, FillAnim::Pulse))
        .Ident("fold_1")
        .Help(
            "Controls input gain and wave folding depth for Channel 1. Turning clockwise amplifies "
            "the waveform past folding thresholds, repeatedly folding signal peaks back inward via "
            "a piecewise linear triangle loop to generate rich harmonic overtone series.");

static VirtualKnob l_symmetry =
    VirtualKnob(kPotMiddleLeft, "Sym 1")
        .Linear(-1.0f, 1.0f)
        .Ring(Bipolar(kSymmetryPos, kSymmetryNeg, kSymmetryCenter))
        .Ident("symmetry_1")
        .Help(
            "Injects bipolar DC offset bias into Channel 1 before soft saturation and folding. "
            "At noon the folds are balanced. Turning clockwise shifts the waveform upward to fold "
            "positive crests more heavily; turning counter-clockwise favors negative trough folds.");

static VirtualKnob l_warp =
    VirtualKnob(kPotBottomLeft, "Warp 1")
        .Linear(-1.0f, 1.0f)
        .Ring(Bipolar(kWarpPos, kWarpNeg, kWarpCenter))
        .Ident("warp_1")
        .Help(
            "Reshapes Channel 1 waveform slope and inflection before folding using a cubic "
            "polynomial transfer function (`x + warp * (x³ - x)`). Counter-clockwise steepens "
            "zero-crossings and flattens peaks into square-like odd harmonics; clockwise creates "
            "a cubic sigmoid S-curve with pinched center and steep peaks.");

/* Page 1: Right Channel Wave Folder */
static VirtualKnob r_fold =
    VirtualKnob(kPotTopRight, "Fold 2")
        .Linear(0.0f, 1.0f)
        .Ring(Level(kFold, FillAnim::Pulse))
        .Ident("fold_2")
        .Help(
            "Controls input gain and wave folding depth for Channel 2. Turning clockwise amplifies "
            "the waveform past folding thresholds, repeatedly folding signal peaks back inward via "
            "a piecewise linear triangle loop to generate rich harmonic overtone series.");

static VirtualKnob r_symmetry =
    VirtualKnob(kPotMiddleRight, "Sym 2")
        .Linear(-1.0f, 1.0f)
        .Ring(Bipolar(kSymmetryPos, kSymmetryNeg, kSymmetryCenter))
        .Ident("symmetry_2")
        .Help(
            "Injects bipolar DC offset bias into Channel 2 before soft saturation and folding. "
            "At noon the folds are balanced. Turning clockwise shifts the waveform upward to fold "
            "positive crests more heavily; turning counter-clockwise favors negative trough folds.");

static VirtualKnob r_warp =
    VirtualKnob(kPotBottomRight, "Warp 2")
        .Linear(-1.0f, 1.0f)
        .Ring(Bipolar(kWarpPos, kWarpNeg, kWarpCenter))
        .Ident("warp_2")
        .Help(
            "Reshapes Channel 2 waveform slope and inflection before folding using a cubic "
            "polynomial transfer function (`x + warp * (x³ - x)`). Counter-clockwise steepens "
            "zero-crossings and flattens peaks into square-like odd harmonics; clockwise creates "
            "a cubic sigmoid S-curve with pinched center and steep peaks.");

/** Page 2: Left Channel (Ch 1) */
static VirtualKnob l_feedback =
    VirtualKnob(kPotTopLeft, "Feed 1")
        .Linear(0.0f, 1.0f)
        .Ring(Level(kFeedback, FillAnim::Pulse))
        .Ident("feedback_1")
        .Help(
            "Feeds post-folder signal back into Channel 1 input through a ~1.5 ms delay tap, "
            "1.8 kHz damping filter, 20 Hz DC blocker, and soft saturation. Adds dense body resonance, "
            "chaotic sub-octave growls, or sustained self-oscillation.");

static VirtualKnob l_distortion =
    VirtualKnob(kPotMiddleLeft, "Dist 1")
        .Linear(0.0f, 1.0f)
        .Ring(Level(kDistortion, FillAnim::Ripple))
        .Ident("distortion_1")
        .Help(
            "Independent asymmetric overdrive with internal feedback diode emulation and "
            "wet/dry blend for Channel 1. Generates thick, aggressive fuzz harmonics. FX order is "
            "controlled by Dist Routing.");

static VirtualKnob l_filter =
    VirtualKnob(kPotBottomLeft, "Filter 1")
        .Linear(-1.0f, 1.0f)
        .Ring(Level(kFilter))
        .Ident("filter_1")
        .Help(
            "Bipolar 12 dB/oct DJ-style filter for Channel 1. Flat at noon. Counter-clockwise sweeps "
            "a low-pass filter down to 30 Hz to tame harsh harmonics; clockwise sweeps a high-pass "
            "filter up to 16 kHz to carve out low frequencies. Resonance is adjusted on the Q sub-page.");

/** Page 2: Right Channel (Ch 2) */
static VirtualKnob r_feedback =
    VirtualKnob(kPotTopRight, "Feed 2")
        .Linear(0.0f, 1.0f)
        .Ring(Level(kFeedback, FillAnim::Pulse))
        .Ident("feedback_2")
        .Help(
            "Feeds post-folder signal back into Channel 2 input through a ~1.5 ms delay tap, "
            "1.8 kHz damping filter, 20 Hz DC blocker, and soft saturation. Adds dense body resonance, "
            "chaotic sub-octave growls, or sustained self-oscillation.");

static VirtualKnob r_distortion =
    VirtualKnob(kPotMiddleRight, "Dist 2")
        .Linear(0.0f, 1.0f)
        .Ring(Level(kDistortion, FillAnim::Ripple))
        .Ident("distortion_2")
        .Help(
            "Independent asymmetric overdrive with internal feedback diode emulation and "
            "wet/dry blend for Channel 2. Generates thick, aggressive fuzz harmonics. FX order is "
            "controlled by Dist Routing.");

static VirtualKnob r_filter =
    VirtualKnob(kPotBottomRight, "Filter 2")
        .Linear(-1.0f, 1.0f)
        .Ring(Level(kFilter))
        .Ident("filter_2")
        .Help(
            "Bipolar 12 dB/oct DJ-style filter for Channel 2. Flat at noon. Counter-clockwise sweeps "
            "a low-pass filter down to 30 Hz to tame harsh harmonics; clockwise sweeps a high-pass "
            "filter up to 16 kHz to carve out low frequencies. Resonance is adjusted on the Q sub-page.");

/** Page 2 Sub-page (Q Edit) Knobs */
static VirtualKnob q_unused_1 =
    VirtualKnob(kPotTopLeft, "Unused")
        .Ident("q_unused_1")
        .Help("Unused parameter on the Q edit sub-page.");

static VirtualKnob q_unused_2 =
    VirtualKnob(kPotTopRight, "Unused")
        .Ident("q_unused_2")
        .Help("Unused parameter on the Q edit sub-page.");

static VirtualKnob q_unused_3 =
    VirtualKnob(kPotMiddleLeft, "Unused")
        .Ident("q_unused_3")
        .Help("Unused parameter on the Q edit sub-page.");

static VirtualKnob q_unused_4 =
    VirtualKnob(kPotMiddleRight, "Unused")
        .Ident("q_unused_4")
        .Help("Unused parameter on the Q edit sub-page.");

static VirtualKnob l_q =
    VirtualKnob(kPotBottomLeft, "Q 1")
        .Linear(0.0f, 1.0f)
        .Ring(Level(kAmber))
        .Ident("filter_q_1")
        .Help(
            "Adjusts resonance ($Q$) for Channel 1's DJ filter. Ranges from flat Butterworth response "
            "up to self-oscillating peak emphasis. Resonance is tapered near center cutoff to keep "
            "neutral sweeps smooth.");

static VirtualKnob r_q =
    VirtualKnob(kPotBottomRight, "Q 2")
        .Linear(0.0f, 1.0f)
        .Ring(Level(kAmber))
        .Ident("filter_q_2")
        .Help(
            "Adjusts resonance ($Q$) for Channel 2's DJ filter. Ranges from flat Butterworth response "
            "up to self-oscillating peak emphasis. Resonance is tapered near center cutoff to keep "
            "neutral sweeps smooth.");

/** Page 1 Buttons */
static const char* const kInputModeLabels[] = {"Normal", "Stereo Link"};
static const LedPanel::Rgb kInputModeColors[] = {kOff, kBtnStereoLink};

static const char* const kBypassLabels[] = {"Active", "Bypassed"};
static const LedPanel::Rgb kBypassColors[] = {kOff, kBtnBypass};

static VirtualButton p1_link =
    VirtualButton(kButtonB2, "Stereo Link")
        .Ident("stereo_link")
        .Selector(kInputModeLabels)
        .Colors(kInputModeColors)
        .Bind(autophage_dsp::SetInputMode)
        .Help(
            "Selects audio input routing mode. In **Normal**, inputs 1 and 2 operate as independent "
            "parallel channels for stereo or dual-mono sources. In **Stereo Link**, input 1 is "
            "multed to both channels, allowing parallel dual-folder processing of a single mono source.");

static VirtualButton p1_bypass =
    VirtualButton(kButtonB3, "Bypass")
        .Ident("bypassed")
        .Selector(kBypassLabels)
        .Colors(kBypassColors)
        .Bind(autophage_dsp::SetBypassed)
        .Help(
            "Master DSP effect bypass. When bypassed, incoming audio routes directly to outputs "
            "unprocessed and panel LED rings are extinguished.");

/** Page 2 Buttons */
static const char* const kDistRoutingLabels[] = {"Pre-Filter", "Post-Filter"};
static const LedPanel::Rgb kDistRoutingColors[] = {kBtnDistPre, kBtnDistPost};

static VirtualButton p2_dist_routing =
    VirtualButton(kButtonB2, "Dist Routing")
        .Ident("dist_routing")
        .Selector(kDistRoutingLabels)
        .Colors(kDistRoutingColors)
        .Bind(autophage_dsp::SetDistortionRouting)
        .Help(
            "Configures the FX chain processing order. **Pre-Filter** routes overdrive distortion "
            "into the DJ filter to smooth aggressive fuzz overtones. **Post-Filter** routes the filter "
            "into distortion to push resonant filter peaks into hard clipping.");

static VirtualButton p2_q_latch =
    VirtualButton(kButtonB3, "Q Edit")
        .Ident("q_edit")
        .Role(VirtualButton::Role::Modal)
        .Action("tap", "Toggle Q Edit Mode")
        .Help(
            "Toggles between Page 2 (Destroy) and Page 3 (Q). In normal mode, the bottom "
            "row knobs sweep the bipolar DJ filter cutoff frequency. When latched into Q Edit "
            "mode, the bottom knobs calibrate filter resonance (Q).");

/* Hardware & Pager surface instances */
static AlchemyLab hw;
static Pager pager = Pager(kNumPages, kNumPots)
                         .Cycle(hw.buttons[kButtonB1], kPageFold, kPageDestroy)
                         .Latch(hw.buttons[kButtonB3], kPageDestroy, kPageQ);

static Page page1 =
    Page(kPageFold)
        .Name("Fold")
        .Color("#67e8f9")
        .Help(
            "Dual parallel wave folding core inspired by the Serge Wave Multiplier and "
            "Zlob Foldiplier. Features independent folding drive, bipolar DC symmetry "
            "offset, and cubic polynomial warp shaping per channel.")
        .Knobs(l_fold, l_symmetry, l_warp, r_fold, r_symmetry, r_warp)
        .Buttons(p1_link, p1_bypass);

static Page page2 =
    Page(kPageDestroy)
        .Name("Destroy")
        .Color("#f75757")
        .Help(
            "Dedicated post-folder tone destruction chains for Channel 1 and Channel 2, "
            "featuring analog-modeled damped feedback, overdrive distortion, "
            "and bipolar DJ filters.")
        .Knobs(l_feedback, l_distortion, l_filter, r_feedback, r_distortion, r_filter)
        .Buttons(p2_dist_routing, p2_q_latch);

static Page page2_q =
    Page(kPageQ)
        .Name("Q")
        .Color("#ffffff")
        .Help(
            "Filter resonance adjustment for the Channel 1 and Channel 2 DJ filters, "
            "accessed by latching B3 on the Destroy page.")
        .Knobs(q_unused_1, q_unused_2, q_unused_3, q_unused_4, l_q, r_q);

/* Remaining surfaces and ControlLoop */
static ControlLoop loop(hw);
static ParamLock<kNumPages * kNumPots, LockLength<20, 20>> locks(hw.buttons[kButtonB1], pager);
static ButtonBank buttons;
static Presets presets(hw.seed.qspi);
static Settings settings(hw, &pager);
static CvMatrix cv_matrix(kNumCvInputs);

static hostlink::Host host(presets, "autophage", "Autophage Wave Folder",
                           "0.1.0", "Alpha1");

static const Manual kManual =
    Manual()
        .Tagline("Dual parallel wave folder with feedback, distortion, and filter")
        .Preamble(
            "Autophage is a dual parallel wave folder firmware for the Hermetic Modular "
            "Alchemy Lab, heavily inspired by the Zlob Foldiplier and Serge Wave Multiplier. "
            "It features two parallel independent wave folders with symmetry offset and "
            "cubic polynomial warping, paired with an analog-modeled damped feedback loop, "
            "overdrive distortion, and a bipolar DJ-style filter.")
        .Section("signal-flow", "Signal Flow",
                 "Audio enters via Jack 1 and Jack 2 (or multed from Jack 1 in Stereo Link mode). "
                 "Each channel sums incoming audio with a damped feedback loop (~1.5 ms delay, 1.8 kHz LPF, "
                 "DC blocker, and tanh saturation) before applying cubic polynomial warp shaping and DC symmetry offset. "
                 "The signal is then driven into a piecewise linear triangle wavefolding loop. "
                 "Folded audio passes through an asymmetric distortion and a bipolar DJ filter in user-selectable order.")
        .PresetsHelp("A preset captures every knob on every page, button toggles, distortion routing, and filter resonance.");

static void InitManual() {
    /* Cross-references between related controls */
    l_fold.SeeAlso(l_symmetry, l_warp, r_fold, l_feedback);
    r_fold.SeeAlso(r_symmetry, r_warp, l_fold, r_feedback);
    l_symmetry.SeeAlso(l_fold, l_warp, r_symmetry);
    r_symmetry.SeeAlso(r_fold, r_warp, l_symmetry);
    l_warp.SeeAlso(l_fold, l_symmetry, r_warp);
    r_warp.SeeAlso(r_fold, r_symmetry, l_warp);

    l_feedback.SeeAlso(l_fold, l_distortion, r_feedback);
    r_feedback.SeeAlso(r_fold, r_distortion, l_feedback);
    l_distortion.SeeAlso(p2_dist_routing, l_filter, l_feedback, r_distortion);
    r_distortion.SeeAlso(p2_dist_routing, r_filter, r_feedback, l_distortion);
    l_filter.SeeAlso(l_q, p2_dist_routing, l_distortion, r_filter);
    r_filter.SeeAlso(r_q, p2_dist_routing, r_distortion, l_filter);
    l_q.SeeAlso(l_filter, r_q, p2_q_latch);
    r_q.SeeAlso(r_filter, l_q, p2_q_latch);

    p1_link.SeeAlso(p1_bypass, l_fold, r_fold);
    p1_bypass.SeeAlso(p1_link, l_fold, r_fold);
    p2_dist_routing.SeeAlso(l_distortion, r_distortion, l_filter, r_filter);
    p2_q_latch.SeeAlso(l_filter, r_filter, l_q, r_q);
}

static void OnRender(uint32_t t_ms) {
    if (autophage_dsp::GetBypassed()) {
        for (uint8_t i = 0; i < kNumPots; i++) {
            hw.leds.ClearRing(i);
        }
    }
}

static void UpdateCoeffs() {
    autophage_dsp::SetBypassed(p1_bypass.Value());
    autophage_dsp::SetInputMode(static_cast<autophage_dsp::InputMode>(p1_link.Zone()));
    autophage_dsp::SetDistortionRouting(static_cast<autophage_dsp::DistortionRouting>(p2_dist_routing.Zone()));

    autophage_dsp::SetChannel(0, {l_fold.Value(),
                                  l_symmetry.Value(),
                                  l_warp.Value(),
                                  l_feedback.Value(),
                                  l_distortion.Value(),
                                  l_filter.Value(),
                                  l_q.Value()});

    autophage_dsp::SetChannel(1, {r_fold.Value(),
                                  r_symmetry.Value(),
                                  r_warp.Value(),
                                  r_feedback.Value(),
                                  r_distortion.Value(),
                                  r_filter.Value(),
                                  r_q.Value()});
}

int main() {
    hw.Init();
    autophage_dsp::Init(hw.SampleRate());

    InitManual();
    host.Attach(kManual);

    static const float kZeroPhys[kNumPots] = {};

    // Set default values for Page 1 knobs
    pager.SetStored(kPageFold, kPotTopLeft, 0.0f, kZeroPhys);      // Fold 1 (norm 0.0 = 0.0f, fully CCW)
    pager.SetStored(kPageFold, kPotTopRight, 0.0f, kZeroPhys);     // Fold 2 (norm 0.0 = 0.0f, fully CCW)
    pager.SetStored(kPageFold, kPotMiddleLeft, 0.5f, kZeroPhys);   // Sym 1 (norm 0.5 = 0.0f, 12 o'clock)
    pager.SetStored(kPageFold, kPotMiddleRight, 0.5f, kZeroPhys);  // Sym 2 (norm 0.5 = 0.0f, 12 o'clock)
    pager.SetStored(kPageFold, kPotBottomLeft, 0.5f, kZeroPhys);   // Warp 1 (norm 0.5 = 0.0f, 12 o'clock)
    pager.SetStored(kPageFold, kPotBottomRight, 0.5f, kZeroPhys);  // Warp 2 (norm 0.5 = 0.0f, 12 o'clock)

    // Set default values for Page 2 knobs
    pager.SetStored(kPageDestroy, kPotTopLeft, 0.0f, kZeroPhys);      // Feed 1
    pager.SetStored(kPageDestroy, kPotTopRight, 0.0f, kZeroPhys);     // Feed 2
    pager.SetStored(kPageDestroy, kPotMiddleLeft, 0.0f, kZeroPhys);   // Dist 1
    pager.SetStored(kPageDestroy, kPotMiddleRight, 0.0f, kZeroPhys);  // Dist 2
    pager.SetStored(kPageDestroy, kPotBottomLeft, 0.5f, kZeroPhys);   // Filter 1 (norm 0.5 = 0.0f, 12 o'clock)
    pager.SetStored(kPageDestroy, kPotBottomRight, 0.5f, kZeroPhys);  // Filter 2 (norm 0.5 = 0.0f, 12 o'clock)

    // Set default values for Page 2 Sub-page (Q Edit) knobs
    pager.SetStored(kPageQ, kPotTopLeft, 0.0f, kZeroPhys);      // Unused 1
    pager.SetStored(kPageQ, kPotTopRight, 0.0f, kZeroPhys);     // Unused 2
    pager.SetStored(kPageQ, kPotMiddleLeft, 0.0f, kZeroPhys);   // Unused 3
    pager.SetStored(kPageQ, kPotMiddleRight, 0.0f, kZeroPhys);  // Unused 4
    pager.SetStored(kPageQ, kPotBottomLeft, 0.2f, kZeroPhys);   // Q 1 (norm 0.2 = default Q)
    pager.SetStored(kPageQ, kPotBottomRight, 0.2f, kZeroPhys);  // Q 2 (norm 0.2 = default Q)

    /* CV routing. Map the 6 CV jacks to the 6 wave folder parameters. */
    cv_matrix.Jack(0).To(l_fold);
    cv_matrix.Jack(1).To(r_fold);
    cv_matrix.Jack(2).To(l_symmetry);
    cv_matrix.Jack(3).To(r_symmetry);
    cv_matrix.Jack(4).To(l_warp);
    cv_matrix.Jack(5).To(r_warp);

    /* Opting into default settings gestures and controls.*/
    settings.UseBrightness();
    settings.UsePresets(presets);

    /* Preset payload — every Serializable surface gets walked on Save/Load. */
    presets.Manage(pager);
    presets.Manage(locks);
    presets.Manage(settings);
    presets.Manage(buttons);
    presets.UseNames();

    /* ControlLoop is a thin, opt-in driver for the canonical control-rate frame. */
    loop.Use(pager)
        .Use(locks)
        .Use(settings)
        .Use(cv_matrix)
        .Use(buttons)
        .Use(page1)
        .Use(page2)
        .Use(page2_q)
        .Use(host)
        .OnFrame(UpdateCoeffs)
        .OnRender(OnRender);

    presets.Init();
    presets.BootLoad();

    UpdateCoeffs();
    hw.StartAudio(autophage_dsp::Process);

    for (;;) loop.Tick();
}
