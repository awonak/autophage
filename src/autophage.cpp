/**
 * autophage.cpp — Alchemy Lab dual mono wave folder.
 */

#include "alchemy/host_link/host.h"
#include "alchemy/hw/alchemy_lab.h"
#include "alchemy/hw/alchemy_lab_v2_layout.h"
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

static VirtualKnob l_offset = VirtualKnob(kPotBottomLeft, "Offset 1")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Bipolar(kOffsetPos, kOffsetNeg, kOffsetCenter));

static VirtualKnob l_symmetry = VirtualKnob(kPotBottomLeft, "Sym 1")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kSymmetry, FillAnim::None));

/* Page 1: Right Channel Wave Folder */
static VirtualKnob r_fold = VirtualKnob(kPotTopRight, "Fold 2")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFold, FillAnim::Pulse));

static VirtualKnob r_offset = VirtualKnob(kPotMiddleRight, "Offset 2")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Bipolar(kOffsetPos, kOffsetNeg, kOffsetCenter));

static VirtualKnob r_symmetry = VirtualKnob(kPotBottomRight, "Sym 2")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kSymmetry, FillAnim::None));

/** Page 2: Feedback (Global) **/
static VirtualKnob p2_feedback = VirtualKnob(kPotTopLeft, "Feedback")
                                     .Linear(0.0f, 1.0f)
                                     .Ring(Level(kFeedback, FillAnim::Pulse));

static VirtualKnob p2_fb_time = VirtualKnob(kPotMiddleLeft, "Delay Time")
                                    .Exp(0.001f, 0.050f)
                                    .Ring(Level(kFeedback, FillAnim::None));

/** Page 2: Distortion (Global) */
static VirtualKnob p2_distortion = VirtualKnob(kPotTopRight, "Distortion")
                                       .Ident("dist.amount")
                                       .Linear(0.0f, 1.0f)
                                       .Ring(Level(kDistortion, FillAnim::Ripple));

static VirtualKnob p2_dist_bias = VirtualKnob(kPotMiddleRight, "Distortion Bias")
                                      .Ident("dist.bias")
                                      .Linear(-1.0f, 1.0f)
                                      .Unit("%")
                                      .Ring(Bipolar(kDistortion, kDistortion, kOffsetCenter));

/** Page 2: Filter (Global) */
static VirtualKnob p2_cutoff = VirtualKnob(kPotBottomLeft, "Cutoff")
                                   .Ident("flt.cutoff")
                                   .Exp(60.0f, 16000.0f)
                                   .Unit("Hz")
                                   .Ring(Level(kFilter, FillAnim::None));

static VirtualKnob p2_res = VirtualKnob(kPotBottomRight, "Resonance")
                                .Ident("flt.resonance")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFilter, FillAnim::None));

/** Page 1 Buttons */
static const char* const kInputModeLabels[] = {"Normal", "Stereo Link"};
static const LedPanel::Rgb kInputModeColors[] = {kOff, kBtnStereoLink};

static const char* const kBypassLabels[] = {"Active", "Bypassed"};
static const LedPanel::Rgb kBypassColors[] = {kOff, kBtnBypass};

static VirtualButton p1_link = VirtualButton(kButtonB2, "Stereo Link")
                                   .Ident("input_mode")
                                   .Selector(kInputModeLabels)
                                   .Colors(kInputModeColors)
                                   .Bind(autophage_dsp::SetInputMode);

static VirtualButton p1_bypass = VirtualButton(kButtonB3, "Bypass")
                                     .Ident("bypassed")
                                     .Selector(kBypassLabels)
                                     .Colors(kBypassColors)
                                     .Bind(autophage_dsp::SetBypassed);

/** Page 2 Buttons */
static const char* const kDistRoutingLabels[] = {"Bypass", "Pre-Filter", "Post-Filter"};
static const LedPanel::Rgb kDistRoutingColors[] = {kOff, kBtnDistPre, kBtnDistPost};

static const char* const kFilterModeLabels[] = {"LowPass", "BandPass", "HighPass"};
static const LedPanel::Rgb kFilterModeColors[] = {kBtnFilterLp, kBtnFilterBp, kBtnFilterHp};

static VirtualButton p2_dist_routing = VirtualButton(kButtonB2, "Dist Routing")
                                           .Ident("dist_routing")
                                           .Selector(kDistRoutingLabels)
                                           .Colors(kDistRoutingColors)
                                           .Bind(autophage_dsp::SetDistortionRouting)
                                           .Anchor("dist.amount");

static VirtualButton p2_filter_mode = VirtualButton(kButtonB3, "Filter Mode")
                                          .Ident("filter_mode")
                                          .Selector(kFilterModeLabels)
                                          .Colors(kFilterModeColors)
                                          .Bind(autophage_dsp::SetFilterMode)
                                          .Anchor("flt.resonance");

static Page page1 = Page(0)
                        .Name("Fold")
                        .Color("#67e8f9")
                        .Knobs(l_fold, l_offset, l_symmetry, r_fold, r_offset, r_symmetry)
                        .Buttons(p1_link, p1_bypass);

static Page page2 = Page(1)
                        .Name("Destroy")
                        .Color("#f75757")
                        .Knobs(p2_feedback, p2_distortion, p2_fb_time, p2_dist_bias, p2_cutoff, p2_res)
                        .Buttons(p2_dist_routing, p2_filter_mode);

/* Get our SDK surfaces and opt in to everything */
static AlchemyLab hw;
static ControlLoop loop(hw);
static Pager pager(hw.buttons[0], 2, kNumPots);
static ParamLock<2 * kNumPots> locks(hw.buttons[0], pager);
static ButtonBank buttons;
static Presets presets(hw.seed.qspi);
static Settings settings(hw, &pager);
static CvMatrix cv_matrix(kNumCvInputs);

static hostlink::Host host(presets, "autophage", "Autophage Wave Folder",
                           "0.1.0", "Alpha1");

static void OnRender(uint32_t t_ms) {
    if (autophage_dsp::GetBypassed()) {
        for (uint8_t i = 0; i < kNumPots; i++) {
            hw.leds.ClearRing(i);
        }
    }
}

static void UpdateCoeffs() {
    autophage_dsp::SetChannel(0, {l_fold.Value(),
                                  l_offset.Value(),
                                  l_symmetry.Value(),
                                  p2_feedback.Value(),
                                  p2_fb_time.Value(),
                                  p2_distortion.Value(),
                                  p2_dist_bias.Value(),
                                  p2_cutoff.Value(),
                                  p2_res.Value()});

    autophage_dsp::SetChannel(1, {r_fold.Value(),
                                  r_offset.Value(),
                                  r_symmetry.Value(),
                                  p2_feedback.Value(),
                                  p2_fb_time.Value(),
                                  p2_distortion.Value(),
                                  p2_dist_bias.Value(),
                                  p2_cutoff.Value(),
                                  p2_res.Value()});
}

int main() {
    hw.Init();
    autophage_dsp::Init(hw.SampleRate());

    // Set default values for background page 2 knobs
    pager.SetStored(1, 0, 0.0f, nullptr);  // Feedback
    pager.SetStored(1, 1, 0.0f, nullptr);  // Distortion
    pager.SetStored(1, 2, 0.0f, nullptr);  // Feedback Time (norm 0 = 0.001f)
    pager.SetStored(1, 3, 0.5f, nullptr);  // Dist Bias (norm 0.5 = 0.0f)
    pager.SetStored(1, 4, 1.0f, nullptr);  // Cutoff
    pager.SetStored(1, 5, 0.0f, nullptr);  // Resonance

    /* CV routing. Map the 6 CV jacks to the 6 wave folder parameters. */
    cv_matrix.Jack(0).To(l_fold);
    cv_matrix.Jack(1).To(r_fold);
    cv_matrix.Jack(2).To(l_offset);
    cv_matrix.Jack(3).To(r_offset);
    cv_matrix.Jack(4).To(l_symmetry);
    cv_matrix.Jack(5).To(r_symmetry);

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
        .OnFrame(UpdateCoeffs)
        .OnRender(OnRender);

    presets.Init();
    presets.BootLoad();

    UpdateCoeffs();
    hw.StartAudio(autophage_dsp::Process);

    for (;;) loop.Tick();
}
