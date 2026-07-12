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
#include "daisy_seed.h"

using namespace alchemy;

/* Page 1: Left Channel */
static VirtualKnob l_fold = VirtualKnob(0, "Fold 1")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(LedPanel::Rgb{255, 0, 0}, FillAnim::Pulse));

static VirtualKnob l_offset = VirtualKnob(2, "Offset 1")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Bipolar(LedPanel::Rgb{247, 149, 11},
                                                LedPanel::Rgb{62, 143, 141},
                                                LedPanel::Rgb{255, 255, 255}));

static VirtualKnob l_symmetry = VirtualKnob(4, "Sym 1")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(LedPanel::Rgb{104, 104, 29}, FillAnim::None));

/* Page 1: Right Channel */
static VirtualKnob r_fold = VirtualKnob(1, "Fold 2")
                                .Linear(0.0f, 1.0f)
                                .Ring(Level(LedPanel::Rgb{255, 0, 0}, FillAnim::Pulse));

static VirtualKnob r_offset = VirtualKnob(3, "Offset 2")
                                  .Linear(-1.0f, 1.0f)
                                  .Ring(Bipolar(LedPanel::Rgb{247, 149, 11},
                                                LedPanel::Rgb{62, 143, 141},
                                                LedPanel::Rgb{255, 255, 255}));

static VirtualKnob r_symmetry = VirtualKnob(5, "Sym 2")
                                    .Linear(0.0f, 1.0f)
                                    .Ring(Level(LedPanel::Rgb{104, 104, 29}, FillAnim::None));

/* Get our SDK surfaces and opt in to everything */
static AlchemyLab hw;
static ControlLoop loop(hw);
static Pager pager(hw.buttons[0], 2, kNumPots);
static ParamLock<2 * kNumPots> locks(hw.buttons[0], pager);
static Presets presets(hw.seed.qspi);
static Settings settings(hw, &pager);
static CvMatrix cv_matrix(kNumCvInputs);

static Page page1 = Page(0).Knobs(l_fold, l_offset, l_symmetry, r_fold, r_offset, r_symmetry);
static Page page2 = Page(1);

static void UpdateCoeffs() {
    autophage_dsp::SetChannel(0, {l_fold.Value(),
                                  l_offset.Value(),
                                  l_symmetry.Value()});
    autophage_dsp::SetChannel(1, {r_fold.Value(),
                                  r_offset.Value(),
                                  r_symmetry.Value()});
}

int main() {
    hw.Init();
    autophage_dsp::Init(hw.SampleRate());

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
        .OnFrame(UpdateCoeffs);

    for (;;) loop.Tick();
}
