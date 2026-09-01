# Autophage (Wave Folder, Feedback, Filter, Distortion)

Autophage is a dual parallel wave folder firmware for the [Hermetic Modular Alchemy Lab](https://hermeticmodular.com/modules/alchemy-lab). The folding core is heavily inspired by the Zlob Foldiplier and Serge Wave Multiplier. 

It features two parallel independent wave folders, an integrated feedback loop, an overdrive distortion circuit, and a multi-mode filter.

## Features & Operation

**Wave Folding**: The Fold knob controls the input gain and folding intensity (indicated by the Brick Ember LED arc). Turning the knob clockwise amplifies the waveform beyond normalized threshold limits, causing the signal peaks to fold back on themselves repeatedly via a piecewise linear triangle folding loop to generate rich, complex harmonics.

**Symmetry (DC Offset Bias & Asymmetric Folding)**: The Symmetry knob is a bipolar control (-1.0 to +1.0) that injects a positive or negative DC offset bias into the waveform prior to soft saturation and wavefolding.
* Turning **clockwise** (+1.0) shifts the waveform upwards, causing positive crests to reach folding thresholds earlier and fold more heavily.
* Turning **counter-clockwise** (-1.0) shifts the waveform downwards, forcing negative troughs to fold more aggressively.
* At **noon** (0.0), the signal remains centered, producing perfectly balanced, symmetrical folds.

**Warp (Polynomial Curve & Sigmoid Shaping)**: The Warp knob is a bipolar control (-1.0 to +1.0) that reshapes the incoming waveform's slope and inflection prior to gain scaling and folding using a cubic polynomial transfer function (`x = x + warp * (x³ - x)`):
* Turning **counter-clockwise** (-1.0) steepens the slope through zero-crossings while flattening the peaks, morphing a sine wave into a warm, rounded square-like shape with odd-harmonic overtone presence.
* Turning **clockwise** (+1.0) flattens the center and pulls the slopes into a pronounced cubic sigmoid S-curve on each polarity, pinching the zero-crossing region and creating steepened, sharp peaks.
* At **noon** (0.0), the transfer function is completely linear, leaving the input waveform unwarped.

---

## Audio Inputs

* **Jack 1**: Wave In 1
* **Jack 2**: Wave In 2

## Page 1: Wave Folder

### Buttons
* **B1**: Change Page
* **B2**: Cycle Input Mode
  * Mode 1 (Off): Normal independent inputs (Stereo/Dual Mono)
  * Mode 2 (Pale Green): Input Mult (Mirrors Left input audio to Right channel)
* **B3**: Toggle Bypass (Passes audio input directly to output, bypassing the effect)

### Knobs
* **Knob 1**: Fold 1 (Brick Ember)
* **Knob 2**: Symmetry 1 (Bipolar)
* **Knob 3**: Warp 1 (Bipolar)
* **Knob 4**: Fold 2 (Brick Ember)
* **Knob 5**: Symmetry 2 (Bipolar)
* **Knob 6**: Warp 2 (Bipolar)

## CV Inputs

The 6 CV inputs dynamically map to the Wave Folder parameters:

* **Jack 1**: Fold 1
* **Jack 2**: Fold 2
* **Jack 3**: Symmetry 1
* **Jack 4**: Symmetry 2
* **Jack 5**: Warp 1
* **Jack 6**: Warp 2

## Page 2: Feedback, Distortion, and Filter

The second page provides dedicated per-channel effects for Channel 1 (Left) and Channel 2 (Right):

**Feedback**: Feeds the output of the wave folder back into the input through an analog-modeled 1.8 kHz damping filter, 20 Hz DC blocker, and `tanh` soft-saturation to produce a deep, low-end growl and harmonic self-oscillation.
* **Feedback 1 & 2** independently control the feedback intensity for each channel.

**Distortion**: A gritty Bazz Fuss-inspired overdrive circuit adds heavy harmonic crunch and grit.
* **Distortion 1 & 2** independently control overdrive gain.
* **Button B2** selects whether distortion is placed **Pre-Filter** or **Post-Filter** in the processing chain.

**Filter**: A bipolar state-variable DJ filter that provides seamless spectral sculpting.
* **Filter 1 & 2** provide independent bipolar frequency filtering:
  * **At Noon (0.0)**: Transparent neutral bypass (flat passthrough).
  * **Counter-Clockwise (CCW)**: Low-Pass Filter (LPF) sweeping cutoff from ~16 kHz down to ~60 Hz.
  * **Clockwise (CW)**: High-Pass Filter (HPF) sweeping cutoff from ~40 Hz up to ~14 kHz.
* **Button B3 (Q Edit)**: Toggles Q edit mode for the Filter knobs. When active, turning the Filter knobs adjusts filter resonance ($Q$), indicated by a bright white pip overdrawn on the LED arc while the base filter level fill is hidden.

### Buttons
* **B1**: Change Page
* **B2**: Toggle Distortion Routing
  * Mode 1 (Dim Orange): Pre-Filter Distortion
  * Mode 2 (Dim Brick Ember): Post-Filter Distortion
* **B3**: Toggle Filter Q Edit Mode
  * Off: Normal Filter Sweep (Ring Level)
  * On (White): Filter Q / Resonance Edit (Overdrawn Pip)

### Knobs
* **Knob 1 (TL)**: Feedback 1 (Purple)
* **Knob 2 (ML)**: Distortion 1 (Orange)
* **Knob 3 (BL)**: Filter 1 (Spruce Blue / Q Edit Pip)
* **Knob 4 (TR)**: Feedback 2 (Purple)
* **Knob 5 (MR)**: Distortion 2 (Orange)
* **Knob 6 (BR)**: Filter 2 (Spruce Blue / Q Edit Pip)

---

## Compilation and Requirements

- `git`
- `make`
- `arm-none-eabi-gcc`
- `dfu-util`

**Ubuntu / Debian:**
```sh
sudo apt install git make gcc-arm-none-eabi dfu-util
```

**macOS (Homebrew):**
```sh
brew install git make dfu-util
brew install --cask gcc-arm-embedded
```

## Getting started

```sh
make libdaisy    # build libDaisy once
make             # build the firmware → build/autophage.bin
```

## Flashing

The Alchemy Lab runs a custom bootloader (`DaisyBootloader-AlchemyLabV2`) that serves DFU over the front-panel USB-C port. Connect that port, then put the module in update mode: during the ~2 s window after power-on — the LED rings spin a warm-white comet — press or hold **B3.** The rings switch to a slow breathe, and the module stays in DFU mode until it's flashed or reset. Then run:

```sh
make program-dfu
```

## License

MIT — see [LICENSE](LICENSE). libDaisy is independently MIT-licensed by Electrosmith.
