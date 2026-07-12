/**
 * autophage.cpp — Alchemy Lab dual mono wave folder.
 */

#include "daisy_seed.h"
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

using namespace alchemy;

// We will add VirtualKnobs here during Milestone 4.
// For now, we just initialize the framework and pass through audio.

/* Get our SDK surfaces and opt in to everything */
static AlchemyLab                        hw;
static ControlLoop                       loop    (hw);
static Pager                             pager   (hw.buttons[0], 2, kNumPots);
static ParamLock<2 * kNumPots>           locks   (hw.buttons[0], pager);
static Presets                           presets (hw.seed.qspi);
static Settings                          settings(hw, &pager);
static CvMatrix                          cv_matrix(kNumCvInputs);

static void UpdateCoeffs()
{
    // Dummy update for now, will map knobs later
    autophage_dsp::SetChannel(0, {0.0f, 0.0f, 0.0f});
    autophage_dsp::SetChannel(1, {0.0f, 0.0f, 0.0f});
}

int main()
{
    hw.Init();
    autophage_dsp::Init(hw.SampleRate());

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
        .OnFrame(UpdateCoeffs);

    for (;;) loop.Tick();
}
