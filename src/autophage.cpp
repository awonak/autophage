/**
 * autophage.cpp — Alchemy Lab dual mono wave folder.
 */

#include "alchemy/host_link/host.h"
#include "alchemy/hw/alchemy_lab.h"
#include "alchemy/hw/alchemy_lab_v2_layout.h"
#include "alchemy/led/ring_frame.h"
#include "alchemy/surface/button_bank.h"
#include "alchemy/surface/control_loop.h"
#include "alchemy/surface/cv_matrix.h"
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

/* Page 1: Left Channel Wave Folder */
static VirtualKnob l_fold = VirtualKnob(kPotTopLeft, "Fold 1")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFold, FillAnim::Pulse));

static VirtualKnob l_symmetry = VirtualKnob(kPotMiddleLeft, "Sym 1")
                                    .Linear(-1.0f, 1.0f)
                                    .Ring(Bipolar(kSymmetryPos, kSymmetryNeg, kSymmetryCenter));

static VirtualKnob l_warp = VirtualKnob(kPotBottomLeft, "Warp 1")
                                .Linear(-1.0f, 1.0f)
                                .Ring(Bipolar(kWarpPos, kWarpNeg, kWarpCenter));

/* Page 1: Right Channel Wave Folder */
static VirtualKnob r_fold = VirtualKnob(kPotTopRight, "Fold 2")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFold, FillAnim::Pulse));

static VirtualKnob r_symmetry = VirtualKnob(kPotMiddleRight, "Sym 2")
                                    .Linear(-1.0f, 1.0f)
                                    .Ring(Bipolar(kSymmetryPos, kSymmetryNeg, kSymmetryCenter));

static VirtualKnob r_warp = VirtualKnob(kPotBottomRight, "Warp 2")
                                .Linear(-1.0f, 1.0f)
                                .Ring(Bipolar(kWarpPos, kWarpNeg, kWarpCenter));

/* Page 2 State */
/* Page 2 Filter & Q Sub-layer State */
static bool p2_q_edit_mode = false;
static PotState l_filter_cutoff_state = {0.5f, true, 0};  // Norm 0.5 = 0.0 (noon bypass)
static PotState r_filter_cutoff_state = {0.5f, true, 0};
static PotState l_filter_q_state = {0.2f, true, 0};  // Norm 0.2 = default Q
static PotState r_filter_q_state = {0.2f, true, 0};

static void DrawFilterRing(LedPanel& panel, uint8_t pot,
                           const ArcGeometry& geo, float /*norm*/,
                           uint32_t t_ms, void* /*ctx*/) {
    uint8_t ch = (pot == kPotBottomLeft) ? 0 : 1;
    const PotState& flt_state = (ch == 0) ? l_filter_cutoff_state : r_filter_cutoff_state;
    const PotState& q_state = (ch == 0) ? l_filter_q_state : r_filter_q_state;
    const PotState& active_state = p2_q_edit_mode ? q_state : flt_state;

    RingFrame f;
    f.Begin(geo);

    // 1. Base Filter Level layer (dimmed when the active control is uncaught)
    FillDesc fill;
    fill.color = active_state.caught ? kFilter : LedPanel::Scale(kFilter, 0.30f);
    f.Base(fill, flt_state.stored, t_ms);

    // 2. Filter Q pip (SolidPip on top of the Filter Level layer)
    PipDesc pip;
    pip.color = active_state.caught ? kWhite : LedPanel::Scale(kWhite, 0.50f);
    pip.compose = PipCompose::Add;
    pip.smooth = true;
    f.Pip(Region::Full, pip, q_state.stored);

    // 3. Emit with PotState so that uncaught pot renders the catch pip
    f.Emit(panel, pot, active_state);
}

/** Page 2: Left Channel (Ch 1) */
static VirtualKnob l_feedback = VirtualKnob(kPotTopLeft, "Feed 1")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kFeedback, FillAnim::Pulse));

static VirtualKnob l_distortion = VirtualKnob(kPotMiddleLeft, "Dist 1")
                                      .Linear(0.0f, 1.0f)
                                      .Ring(Level(kDistortion, FillAnim::Ripple));

static VirtualKnob l_filter = VirtualKnob(kPotBottomLeft, "Filter 1")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Custom(DrawFilterRing));

/** Page 2: Right Channel (Ch 2) */
static VirtualKnob r_feedback = VirtualKnob(kPotTopRight, "Feed 2")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kFeedback, FillAnim::Pulse));

static VirtualKnob r_distortion = VirtualKnob(kPotMiddleRight, "Dist 2")
                                      .Linear(0.0f, 1.0f)
                                      .Ring(Level(kDistortion, FillAnim::Ripple));

static VirtualKnob r_filter = VirtualKnob(kPotBottomRight, "Filter 2")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Custom(DrawFilterRing));

/** Page 1 Buttons */
static const char* const kInputModeLabels[] = {"Normal", "Stereo Link"};
static const LedPanel::Rgb kInputModeColors[] = {kOff, kBtnStereoLink};

static const char* const kBypassLabels[] = {"Active", "Bypassed"};
static const LedPanel::Rgb kBypassColors[] = {kOff, kBtnBypass};

static VirtualButton p1_link = VirtualButton(kButtonB2, "Stereo Link")
                                   .Toggle();

static VirtualButton p1_bypass = VirtualButton(kButtonB3, "Bypass")
                                     .Ident("bypassed")
                                     .Selector(kBypassLabels)
                                     .Colors({kOff, kBtnStereoLink})
                                     .Bind(autophage_dsp::SetBypassed);

/** Page 2 Buttons */
static const char* const kDistRoutingLabels[] = {"Pre-Filter", "Post-Filter"};
static const LedPanel::Rgb kDistRoutingColors[] = {kBtnDistPre, kBtnDistPost};

static const char* const kFilterQEditLabels[] = {"Filter", "Q Edit"};
static const LedPanel::Rgb kFilterQEditColors[] = {kOff, kWhite};

/* Hardware & Pager surface instances */
static AlchemyLab hw;
static Pager pager(hw.buttons[0], 2, kNumPots);

static void SetFilterQEdit(bool enable) {
    p2_q_edit_mode = enable;
    if (p2_q_edit_mode) {
        InitCatch(l_filter_q_state, hw.pots[kPotBottomLeft].Value());
        InitCatch(r_filter_q_state, hw.pots[kPotBottomRight].Value());
    } else {
        InitCatch(l_filter_cutoff_state, hw.pots[kPotBottomLeft].Value());
        InitCatch(r_filter_cutoff_state, hw.pots[kPotBottomRight].Value());
    }
}

static VirtualButton p2_dist_routing = VirtualButton(kButtonB2, "Dist Routing")
                                           .Ident("dist_routing")
                                           .Selector(kDistRoutingLabels)
                                           .Colors(kDistRoutingColors)
                                           .Bind(autophage_dsp::SetDistortionRouting);

static VirtualButton p2_filter_q_mode = VirtualButton(kButtonB3, "Q Edit")
                                            .Ident("q_edit")
                                            .Selector(kFilterQEditLabels)
                                            .Colors(kFilterQEditColors)
                                            .Bind(SetFilterQEdit);

static Page page1 = Page(0)
                        .Name("Fold")
                        .Color("#67e8f9")
                        .Knobs(l_fold, l_symmetry, l_warp, r_fold, r_symmetry, r_warp)
                        .Buttons(p1_link, p1_bypass);

static Page page2 = Page(1)
                        .Name("Destroy")
                        .Color("#f75757")
                        .Knobs(l_feedback, l_distortion, l_filter, r_feedback, r_distortion, r_filter)
                        .Buttons(p2_dist_routing, p2_filter_q_mode);

/* Remaining surfaces and ControlLoop */
static ControlLoop loop(hw);
static ParamLock<2 * kNumPots> locks(hw.buttons[0], pager);
static ButtonBank buttons;
static Presets presets(hw.seed.qspi);
static Settings settings(hw, &pager);
static CvMatrix cv_matrix(kNumCvInputs);

static hostlink::Host host(presets, "autophage", "Autophage Wave Folder",
                           "0.1.0", "Alpha1");

static void OnPageChange() {
    if (pager.Page() == 1) {
        InitCatch(l_filter_cutoff_state, hw.pots[kPotBottomLeft].Value());
        InitCatch(r_filter_cutoff_state, hw.pots[kPotBottomRight].Value());
        InitCatch(l_filter_q_state, hw.pots[kPotBottomLeft].Value());
        InitCatch(r_filter_q_state, hw.pots[kPotBottomRight].Value());
    }
}

static void OnRender(uint32_t t_ms) {
    if (autophage_dsp::GetBypassed()) {
        for (uint8_t i = 0; i < kNumPots; i++) {
            hw.leds.ClearRing(i);
        }
    }
}

static void UpdateCoeffs() {
    if (pager.Page() == 1) {
        if (p2_q_edit_mode) {
            UpdateCatch(l_filter_q_state, hw.pots[kPotBottomLeft].Value());
            UpdateCatch(r_filter_q_state, hw.pots[kPotBottomRight].Value());
        } else {
            UpdateCatch(l_filter_cutoff_state, hw.pots[kPotBottomLeft].Value());
            UpdateCatch(r_filter_cutoff_state, hw.pots[kPotBottomRight].Value());
        }
    }

    autophage_dsp::SetBypassed(p1_bypass.Value());

    float l_filter_val = (l_filter_cutoff_state.stored * 2.0f) - 1.0f;
    float r_filter_val = (r_filter_cutoff_state.stored * 2.0f) - 1.0f;

    autophage_dsp::SetChannel(0, {l_fold.Value(),
                                  l_symmetry.Value(),
                                  l_warp.Value(),
                                  l_feedback.Value(),
                                  l_distortion.Value(),
                                  l_filter_val,
                                  l_filter_q_state.stored});

    autophage_dsp::SetChannel(1, {r_fold.Value(),
                                  r_symmetry.Value(),
                                  r_warp.Value(),
                                  r_feedback.Value(),
                                  r_distortion.Value(),
                                  r_filter_val,
                                  r_filter_q_state.stored});
}

int main() {
    hw.Init();
    autophage_dsp::Init(hw.SampleRate());

    static const float kZeroPhys[kNumPots] = {};

    // Set default values for Page 1 knobs
    pager.SetStored(0, 0, 0.0f, kZeroPhys);  // Fold 1 (norm 0.0 = 0.0f, fully CCW)
    pager.SetStored(0, 1, 0.0f, kZeroPhys);  // Fold 2 (norm 0.0 = 0.0f, fully CCW)
    pager.SetStored(0, 2, 0.5f, kZeroPhys);  // Sym 1 (norm 0.5 = 0.0f, 12 o'clock)
    pager.SetStored(0, 3, 0.5f, kZeroPhys);  // Sym 2 (norm 0.5 = 0.0f, 12 o'clock)
    pager.SetStored(0, 4, 0.5f, kZeroPhys);  // Warp 1 (norm 0.5 = 0.0f, 12 o'clock)
    pager.SetStored(0, 5, 0.5f, kZeroPhys);  // Warp 2 (norm 0.5 = 0.0f, 12 o'clock)

    // Set default values for background Page 2 knobs
    pager.SetStored(1, 0, 0.0f, kZeroPhys);  // Feed 1
    pager.SetStored(1, 1, 0.0f, kZeroPhys);  // Feed 2
    pager.SetStored(1, 2, 0.0f, kZeroPhys);  // Dist 1
    pager.SetStored(1, 3, 0.0f, kZeroPhys);  // Dist 2
    pager.SetStored(1, 4, 0.5f, kZeroPhys);  // Filter 1 (norm 0.5 = 0.0f, 12 o'clock)
    pager.SetStored(1, 5, 0.5f, kZeroPhys);  // Filter 2 (norm 0.5 = 0.0f, 12 o'clock)

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
        .Use(host)
        .OnPageChange(OnPageChange)
        .OnFrame(UpdateCoeffs)
        .OnRender(OnRender);

    presets.Init();
    presets.BootLoad();

    UpdateCoeffs();
    hw.StartAudio(autophage_dsp::Process);

    for (;;) loop.Tick();
}
