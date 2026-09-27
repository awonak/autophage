# Autophage

**Wave Folder with Feedback, Filter, and Distortion**

Autophage is a dual parallel wave folder firmware for the [Hermetic Modular Alchemy Lab](https://hermeticmodular.com/modules/alchemy-lab). The folding core is heavily inspired by the Zlob Foldiplier and Serge Wave Multiplier. 

It features two parallel independent wave folders with symmetry offset and cubic polynomial warping. Additional wave shaping includes a soft saturated feedback loop, overdrive distortion, and a bi-polar DJ style filter.

## Quick Start

Head over to the Autophage [Releases](https://github.com/awonak/autophage/releases) page to find the latest release. Download the `.bin` file from the artifacts section and use the Hermetic Modular [Programmer](https://hermeticmodular.com/program) to flash the firmware to your Alchemy Lab.

## Features & Operation

**Page 1 // Fold**

**Wave Folding 1 & 2 (Top-Left / Top-Right)**: The Fold knob controls the input gain and folding intensity (indicated by the Brick Ember LED arc). Turning the knob clockwise amplifies the waveform beyond normalized threshold limits, causing the signal peaks to fold back on themselves repeatedly via a piecewise linear triangle folding loop to generate rich, complex harmonics.

**Symmetry  1 & 2 (Middle-Left / Middle-Right)**: DC Offset Bias & Asymmetric Folding. The Symmetry knob is a bipolar control (-1.0 to +1.0) that injects a positive or negative DC offset bias into the waveform prior to soft saturation and wavefolding.
* Turning **clockwise** (+1.0) shifts the waveform upwards, causing positive crests to reach folding thresholds earlier and fold more heavily.
* Turning **counter-clockwise** (-1.0) shifts the waveform downwards, forcing negative troughs to fold more aggressively.
* At **noon** (0.0), the signal remains centered, producing perfectly balanced, symmetrical folds.

**Warp 1 & 2 (Bottom-Left / Bottom-Right)**: Polynomial Curve & Sigmoid Shaping. The Warp knob is a bipolar control (-1.0 to +1.0) that reshapes the incoming waveform's slope and inflection prior to gain scaling and folding using a cubic polynomial transfer function (`x = x + warp * (x³ - x)`):
* Turning **counter-clockwise** (-1.0) steepens the slope through zero-crossings while flattening the peaks, morphing a sine wave into a warm, rounded square-like shape with odd-harmonic overtone presence.
* Turning **clockwise** (+1.0) flattens the center and pulls the slopes into a pronounced cubic sigmoid S-curve on each polarity, pinching the zero-crossing region and creating steepened, sharp peaks.
* At **noon** (0.0), the transfer function is completely linear, leaving the input waveform unwarped.

**Page 2 // Destroy**

**Feedback 1 & 2 (Top-Left / Top-Right)**: Feeds post-folder signal back into the input through a 1.8 kHz damping filter, 20 Hz DC blocker, and soft saturation to create deep, sub-octave growls or self-oscillation.

**Distortion 1 & 2 (Middle-Left / Middle-Right)**: Independent overdrive distortion. Fully counter-clockwise (0.0) is clean/bypassed; turning clockwise increases saturation gain and blends in fuzz harmonics. Pressing B3 will toggle the behavior of applying distortion pre-filter (orange) or post-filter (red).

**Filter 1 & 2 (Bottom-Left / Bottom-Right)**:
  * **Normal Mode (B3 Off)**: Bipolar DJ filter sweep:
    * **At Noon (0.0)**: Neutral bypass (flat frequency response).
    * **Counter-Clockwise (< 0.0)**: Low-Pass Filter (LPF) sweeping cutoff from 20 kHz down to 30 Hz.
    * **Clockwise (> 0.0)**: High-Pass Filter (HPF) sweeping cutoff from 20 Hz up to 16 kHz.
  * **Q Edit Mode (B3 White)**: Turning the knob adjusts filter resonance ($Q$), indicated by a white pip on the LED ring.


## Audio Inputs

* **Jack 1**: Wave In 1
* **Jack 2**: Wave In 2

## CV Inputs

The 6 CV inputs dynamically map to the Wave Folder parameters:

* **Jack 1**: Fold 1
* **Jack 2**: Fold 2
* **Jack 3**: Symmetry 1
* **Jack 4**: Symmetry 2
* **Jack 5**: Warp 1
* **Jack 6**: Warp 2


## Page 1: Fold (Wave Folder, Symmetry, Warp)

### Knobs

| Physical Knob | Channel 1 (Left) | Channel 2 (Right) | Description |
| :--- | :--- | :--- | :--- |
| **Top** | **Fold 1** *(Brick Ember)* | **Fold 2** *(Brick Ember)* | ~5x amplified gain wave folding |
| **Middle** | **Symmetry 1** *(Pale Green / Spruce Blue)* | **Symmetry 2** *(Pale Green / Spruce Blue)* | Bi-polar DC offset prior to folding |
| **Bottom** | **Warp 1** *(Steel Blue / Amber)* | **Warp 2** *(Steel Blue / Amber)* | Cubic polynomial warping |


### Buttons
* **B1**: Change Page
* **B2**: Cycle Input Mode
  * Mode 1 (Off): Normal independent inputs (Stereo/Dual Mono)
  * Mode 2 (Pale Green): Input Mult (Mirrors Left input audio to Right channel)
* **B3**: Toggle Bypass (Passes audio input directly to output, bypassing the effect)

## Page 2: Destroy (Feedback, Distortion, and Filter)

Page 2 provides dedicated, independent processing chains for **Channel 1 (Left)** and **Channel 2 (Right)**:

| Physical Knob | Channel 1 (Left) | Channel 2 (Right) | Description |
| :--- | :--- | :--- | :--- |
| **Top** | **Feedback 1** *(Purple)* | **Feedback 2** *(Purple)* | Analog-modeled feedback with damping and soft saturation |
| **Middle** | **Dist 1** *(Orange)* | **Dist 2** *(Orange)* | Gritty Bazz Fuss overdrive drive and wet/dry blend |
| **Bottom** | **Filter 1** *(Spruce Blue)* | **Filter 2** *(Spruce Blue)* | Bipolar DJ filter / Q resonance (Cutoff or Q depending on B3) |


### Buttons

* **B1**: Change Page (cycles back to Page 1 *Fold*)
* **B2 — Dist Routing**: Sets the order of the FX chain:
  * `Dim Orange`: **Pre-Filter** (Distortion feeds into the Filter)
  * `Dim Brick Ember`: **Post-Filter** (Filter feeds into the Distortion)
* **B3 — Q Edit**: Toggles resonance editing for the bottom filter knobs:
  * `Off`: Normal Cutoff sweep (base ring fill)
  * `White`: Q / Resonance edit mode (overdrawn white pip)

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
