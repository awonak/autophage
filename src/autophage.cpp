/**
 * autophage.cpp — Alchemy Lab dual mono wave folder.
 */

#include "alchemy/host_link/host.h"
#include "alchemy/hw/alchemy_lab.h"
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
static VirtualKnob l_fold = VirtualKnob(0, "Fold 1")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFold, FillAnim::Pulse));

static VirtualKnob l_offset = VirtualKnob(2, "Offset 1")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Bipolar(kOffsetPos, kOffsetNeg, kOffsetCenter));

static VirtualKnob l_symmetry = VirtualKnob(4, "Sym 1")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kSymmetry, FillAnim::None));

/* Page 1: Right Channel Wave Folder */
static VirtualKnob r_fold = VirtualKnob(1, "Fold 2")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFold, FillAnim::Pulse));

static VirtualKnob r_offset = VirtualKnob(3, "Offset 2")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Bipolar(kOffsetPos, kOffsetNeg, kOffsetCenter));

static VirtualKnob r_symmetry = VirtualKnob(5, "Sym 2")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kSymmetry, FillAnim::None));

/** Page 2: Feedback (Global) **/
static VirtualKnob p2_feedback = VirtualKnob(0, "Feedback")
                                     .Linear(0.0f, 1.0f)
                                     .Ring(Level(kFeedback, FillAnim::Pulse));

static VirtualKnob p2_fb_time = VirtualKnob(2, "Delay Time")
                                    .Exp(0.001f, 0.050f)
                                    .Ring(Level(kFeedback, FillAnim::None));

/** Page 2: Distortion (Global) */
static VirtualKnob p2_distortion = VirtualKnob(1, "Distortion")
                                       .Linear(0.0f, 1.0f)
                                       .Ring(Level(kDistortion, FillAnim::Ripple));

static VirtualKnob p2_dist_bias = VirtualKnob(3, "Dist Bias")
                                      .Linear(-1.0f, 1.0f)
                                      .Ring(Bipolar(kDistortion, kDistortion, kOffsetCenter));

/** Page 2: Filter (Global) */
static VirtualKnob p2_cutoff = VirtualKnob(4, "Cutoff")
                                   .Exp(60.0f, 16000.0f)
                                   .Ring(Level(kFilter, FillAnim::None));

static VirtualKnob p2_res = VirtualKnob(5, "Resonance")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(kFilter, FillAnim::None));

static constexpr VirtualButton kButtons[] = {
    /** Page 1 - Fold */
    VirtualButton("p1b1", "Page / Lock")
        .Role(VirtualButton::Role::Modal)
        .Action("tap", "Next Page")
        .Action("hold+knob", "Record / Nudge Param Lock"),
    VirtualButton("p1b2", "Stereo Link")
        .Role(VirtualButton::Role::State)
        .Action("tap", "Toggle Mode"),
    VirtualButton("p1b3", "Bypass")
        .Role(VirtualButton::Role::State)
        .Action("tap", "Toggle Effect"),
    /** Page 2 - Destroy */
    VirtualButton("p2b1", "Page / Lock")
        .Role(VirtualButton::Role::Modal)
        .Action("tap", "Next Page")
        .Action("hold+knob", "Record / Nudge Param Lock"),
    VirtualButton("p2b2", "Dist Routing")
        .Role(VirtualButton::Role::State)
        .Action("tap", "Cycle Mode Pre-Filter / Post-Filter"),
    VirtualButton("p2b3", "Filter Mode")
        .Role(VirtualButton::Role::State)
        .Action("tap", "Cycle Mode")};

static Page page1 = Page(0).Name("Fold").Color("#67e8f9").Knobs(l_fold, l_offset, l_symmetry, r_fold, r_offset, r_symmetry);
static Page page2 = Page(1).Name("Destroy").Color("#f75757").Knobs(p2_feedback, p2_distortion, p2_fb_time, p2_dist_bias, p2_cutoff, p2_res);

/* Get our SDK surfaces and opt in to everything */
static AlchemyLab hw;
static ControlLoop loop(hw);
static Pager pager(hw.buttons[0], 2, kNumPots);
static ParamLock<2 * kNumPots> locks(hw.buttons[0], pager);
static Presets presets(hw.seed.qspi);
static Settings settings(hw, &pager);
static CvMatrix cv_matrix(kNumCvInputs);

static hostlink::Host host(presets, "autophage", "Autophage Wave Folder",
                           "0.1.0", "Alpha1");

static LedPanel::Rgb FilterModeColor(autophage_dsp::FilterMode mode) {
    switch (mode) {
        case autophage_dsp::FilterMode::LowPass:
            return kBtnFilterLp;
        case autophage_dsp::FilterMode::BandPass:
            return kBtnFilterBp;
        case autophage_dsp::FilterMode::HighPass:
            return kBtnFilterHp;
        default:
            return kOff;
    }
}

static LedPanel::Rgb DistRoutingColor(autophage_dsp::DistortionRouting routing) {
    switch (routing) {
        case autophage_dsp::DistortionRouting::PreFilter:
            return kBtnDistPre;
        case autophage_dsp::DistortionRouting::PostFilter:
            return kBtnDistPost;
        default:
            return kOff;
    }
}

static void OnRender(uint32_t t_ms) {
    if (autophage_dsp::GetBypassed()) {
        for (uint8_t i = 0; i < kNumPots; i++) {
            hw.leds.ClearRing(i);
        }
        hw.leds.SetButtonPair(kButtonB3, kBtnBypass);
        return;
    }

    if (pager.ActivePage() == 0) {
        const auto link_color = (autophage_dsp::GetInputMode() == autophage_dsp::InputMode::StereoLink)
                                    ? kBtnStereoLink
                                    : kOff;
        hw.leds.SetButtonPair(kButtonB2, link_color);
        hw.leds.SetButtonPair(kButtonB3, kOff);
    } else if (pager.ActivePage() == 1) {
        hw.leds.SetButtonPair(kButtonB2, DistRoutingColor(autophage_dsp::GetDistortionRouting()));
        hw.leds.SetButtonPair(kButtonB3, FilterModeColor(autophage_dsp::GetFilterMode()));
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
    presets.UseNames();

    /* ControlLoop is a thin, opt-in driver for the canonical control-rate frame. */
    loop.Use(pager)
        .Use(locks)
        .Use(settings)
        .Use(cv_matrix)
        .Use(page1)
        .Use(page2)
        .Use(host)
        .OnFrame(UpdateCoeffs)
        .OnRender(OnRender);

    host.Buttons(kButtons, std::size(kButtons));

    presets.Init();
    presets.BootLoad();

    UpdateCoeffs();
    hw.StartAudio(autophage_dsp::Process);

    for (;;) loop.Tick();
}
