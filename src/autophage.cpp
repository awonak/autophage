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

enum : uint8_t {
    kPageFold = 0,
    kPageDestroy = 1,
    kPageQ = 2,
    kNumPages = 3,
};

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

/** Page 2: Left Channel (Ch 1) */
static VirtualKnob l_feedback = VirtualKnob(kPotTopLeft, "Feed 1")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kFeedback, FillAnim::Pulse));

static VirtualKnob l_distortion = VirtualKnob(kPotMiddleLeft, "Dist 1")
                                      .Linear(0.0f, 1.0f)
                                      .Ring(Level(kDistortion, FillAnim::Ripple));

static VirtualKnob l_filter = VirtualKnob(kPotBottomLeft, "Filter 1")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Level(kFilter));

/** Page 2: Right Channel (Ch 2) */
static VirtualKnob r_feedback = VirtualKnob(kPotTopRight, "Feed 2")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(kFeedback, FillAnim::Pulse));

static VirtualKnob r_distortion = VirtualKnob(kPotMiddleRight, "Dist 2")
                                      .Linear(0.0f, 1.0f)
                                      .Ring(Level(kDistortion, FillAnim::Ripple));

static VirtualKnob r_filter = VirtualKnob(kPotBottomRight, "Filter 2")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Level(kFilter));

/** Page 2 Sub-page (Q Edit) Knobs */
static VirtualKnob l_q = VirtualKnob(kPotBottomLeft, "Q 1")
                             .Linear(0.0f, 1.0f)
                             .Ring(Level(kAmber));

static VirtualKnob r_q = VirtualKnob(kPotBottomRight, "Q 2")
                             .Linear(0.0f, 1.0f)
                             .Ring(Level(kAmber));

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

static VirtualButton p2_dist_routing = VirtualButton(kButtonB2, "Dist Routing")
                                           .Ident("dist_routing")
                                           .Selector(kDistRoutingLabels)
                                           .Colors(kDistRoutingColors)
                                           .Bind(autophage_dsp::SetDistortionRouting);

/* Hardware & Pager surface instances */
static AlchemyLab hw;
static Pager pager = Pager(kNumPages, kNumPots)
                         .Cycle(hw.buttons[kButtonB1], kPageFold, kPageDestroy)
                         .Latch(hw.buttons[kButtonB3], kPageDestroy, kPageQ);

static Page page1 = Page(kPageFold)
                        .Name("Fold")
                        .Color("#67e8f9")
                        .Knobs(l_fold, l_symmetry, l_warp, r_fold, r_symmetry, r_warp)
                        .Buttons(p1_link, p1_bypass);

static Page page2 = Page(kPageDestroy)
                        .Name("Destroy")
                        .Color("#f75757")
                        .Knobs(l_feedback, l_distortion, l_filter, r_feedback, r_distortion, r_filter)
                        .Buttons(p2_dist_routing);

static Page page2_q = Page(kPageQ)
                          .Name("Q")
                          .Color("#ffffff")
                          .Knobs(l_q, r_q);

/* Remaining surfaces and ControlLoop */
static ControlLoop loop(hw);
static ParamLock<kNumPages * kNumPots, LockLength<20, 20>> locks(hw.buttons[kButtonB1], pager);
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
    autophage_dsp::SetBypassed(p1_bypass.Value());

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
