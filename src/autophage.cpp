/**
 * autophage.cpp — Alchemy Lab dual mono wave folder.
 */

#include "alchemy/hw/alchemy_lab.h"
#include "alchemy/surface/control_loop.h"
#include "alchemy/surface/cv_matrix.h"
#include "alchemy/surface/page.h"
#include "alchemy/surface/pager.h"
#include "alchemy/surface/param_lock.h"
#include "alchemy/surface/presets.h"
#include "alchemy/surface/settings.h"
#include "alchemy/surface/virtual_knob.h"
#include "autophage_dsp.h"
#include "autophage_palette.h"
#include "daisy_seed.h"

using namespace alchemy;
using namespace autophage::palette;

/* Page 1: Left Channel */
static VirtualKnob l_fold = VirtualKnob(0, "Fold 1")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFold, FillAnim::Pulse));

static VirtualKnob l_offset = VirtualKnob(2, "Offset 1")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Bipolar(kOffsetPos, kOffsetNeg, kOffsetCenter));

static VirtualKnob l_symmetry = VirtualKnob(4, "Sym 1")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kSymmetry, FillAnim::None));

/* Page 1: Right Channel */
static VirtualKnob r_fold = VirtualKnob(1, "Fold 2")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFold, FillAnim::Pulse));

static VirtualKnob r_offset = VirtualKnob(3, "Offset 2")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Bipolar(kOffsetPos, kOffsetNeg, kOffsetCenter));

static VirtualKnob r_symmetry = VirtualKnob(5, "Sym 2")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kSymmetry, FillAnim::None));

/* Get our SDK surfaces and opt in to everything */
static AlchemyLab hw;
static ControlLoop loop(hw);
static Pager pager(hw.buttons[0], 2, kNumPots);
static ParamLock<2 * kNumPots> locks(hw.buttons[0], pager);
static Presets presets(hw.seed.qspi);
static Settings settings(hw, &pager);
static CvMatrix cv_matrix(kNumCvInputs);

static VirtualKnob p2_feedback = VirtualKnob(0, "Feedback")
                                     .Linear(0.0f, 1.0f)
                                     .Ring(Level(kFeedback, FillAnim::Pulse));

static VirtualKnob p2_distortion = VirtualKnob(1, "Distortion")
                                       .Linear(0.0f, 1.0f)
                                       .Ring(Level(kDistortion, FillAnim::Ripple));

static VirtualKnob p2_fb_time = VirtualKnob(2, "Delay Time")
                                    .Exp(0.001f, 0.050f)
                                    .Ring(Level(kFeedback, FillAnim::None));

static VirtualKnob p2_dist_bias = VirtualKnob(3, "Dist Bias")
                                      .Linear(-1.0f, 1.0f)
                                      .Ring(Bipolar(kDistortion, kDistortion, kOffsetCenter));

static VirtualKnob p2_cutoff = VirtualKnob(4, "Cutoff")
                                   .Exp(60.0f, 16000.0f)
                                   .Ring(Level(kFilter, FillAnim::None));

static VirtualKnob p2_res = VirtualKnob(5, "Resonance")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFilter, FillAnim::None));

static Page page1 = Page(0).Knobs(l_fold, l_offset, l_symmetry, r_fold, r_offset, r_symmetry);
static Page page2 = Page(1).Knobs(p2_feedback, p2_distortion, p2_fb_time, p2_dist_bias, p2_cutoff, p2_res);

static void OnRender(uint32_t t_ms) {
    if (autophage_dsp::GetBypassed()) {
        for (uint8_t i = 0; i < kNumPots; i++) {
            hw.leds.ClearRing(i);
        }
        hw.leds.SetButtonPair(kButtonB3, kBtnBypass);
    } else {
        if (pager.ActivePage() == 0) {
            if (autophage_dsp::GetInputMode() == autophage_dsp::InputMode::StereoLink) {
                hw.leds.SetButtonPair(kButtonB2, kBtnStereoLink);
            } else {
                hw.leds.SetButtonPair(kButtonB2, kOff);
            }
            hw.leds.SetButtonPair(kButtonB3, kOff);
        } else if (pager.ActivePage() == 1) {
            if (autophage_dsp::GetDistortionRouting() == autophage_dsp::DistortionRouting::PreFilter) {
                hw.leds.SetButtonPair(kButtonB2, kBtnDistPre);
            } else if (autophage_dsp::GetDistortionRouting() == autophage_dsp::DistortionRouting::PostFilter) {
                hw.leds.SetButtonPair(kButtonB2, kBtnDistPost);
            } else {
                hw.leds.SetButtonPair(kButtonB2, kOff);
            }

            if (autophage_dsp::GetFilterMode() == autophage_dsp::FilterMode::LowPass) {
                hw.leds.SetButtonPair(kButtonB3, kBtnFilterLp);
            } else if (autophage_dsp::GetFilterMode() == autophage_dsp::FilterMode::BandPass) {
                hw.leds.SetButtonPair(kButtonB3, kBtnFilterBp);
            } else if (autophage_dsp::GetFilterMode() == autophage_dsp::FilterMode::HighPass) {
                hw.leds.SetButtonPair(kButtonB3, kBtnFilterHp);
            } else {
                hw.leds.SetButtonPair(kButtonB3, kOff);
            }
        }
    }
}

static void UpdateCoeffs() {
    // Handle button logic
    if (pager.ActivePage() == 0) {
        if (hw.buttons[1].RisingEdge()) {
            int next = (static_cast<int>(autophage_dsp::GetInputMode()) + 1) % static_cast<int>(autophage_dsp::InputMode::NumModes);
            autophage_dsp::SetInputMode(static_cast<autophage_dsp::InputMode>(next));
        }
        if (hw.buttons[2].RisingEdge()) {
            autophage_dsp::SetBypassed(!autophage_dsp::GetBypassed());
        }
    } else if (pager.ActivePage() == 1) {
        if (hw.buttons[1].RisingEdge()) {
            int next = (static_cast<int>(autophage_dsp::GetDistortionRouting()) + 1) % static_cast<int>(autophage_dsp::DistortionRouting::NumModes);
            autophage_dsp::SetDistortionRouting(static_cast<autophage_dsp::DistortionRouting>(next));
        }
        if (hw.buttons[2].RisingEdge()) {
            int next = (static_cast<int>(autophage_dsp::GetFilterMode()) + 1) % static_cast<int>(autophage_dsp::FilterMode::NumModes);
            autophage_dsp::SetFilterMode(static_cast<autophage_dsp::FilterMode>(next));
        }
    }

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
    presets.Init();
    presets.BootLoad();

    UpdateCoeffs();
    hw.StartAudio(autophage_dsp::Process);

    /* ControlLoop is a thin, opt-in driver for the canonical control-rate frame. */
    loop.Use(pager)
        .Use(locks)
        .Use(settings)
        .Use(cv_matrix)
        .Use(page1)
        .Use(page2)
        .OnFrame(UpdateCoeffs)
        .OnRender(OnRender);

    for (;;) loop.Tick();
}
