# Autophage (Wave Folder, Feedback, Filter, Distortion)

Autophage is a dual parallel wave folder firmware for the [Hermetic Modular Alchemy Lab](https://hermeticmodular.com/modules/alchemy-lab). The folding core is heavily inspired by the Zlob Foldiplier and Serge Wave Multiplier. 

It features two parallel independent wave folders, an integrated feedback loop, an overdrive distortion circuit, and a multi-mode filter.

## Features & Operation

The Fold knob controls the amount of times the waveform is folded in on itself (indicated by the Brick Ember LED arc). The LED arc displays a gradient of Brick Ember, from less intense to more intense, indicating the number of folds.

The Offset knob controls how much positive DC voltage (0 to +5V, clockwise) or negative DC voltage (0 to -5V counter-clockwise) is injected into the waveform via the Symmetry knob. The LED arc for Offset uses Amber for positive values and Space Blue for negative values. With the Offset knob position at noon (0V) there will be little effect from the Symmetry knob. The Offset jack expects +5V to -5V, with the knob position at noon, to scan through the positive and negative voltage.

The Symmetry knob controls the amount of negative or positive offset voltage (dependent on the Offset knob) mixed with the waveform (indicated by the Pale Green LED arc). If positive voltage is mixed with incoming waveform, more folds will occur on the positive troughs of the waveform; likewise, when negative voltage is introduced, more folds happen on the negative troughs.

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
* **Knob 2**: Offset 1 (Amber / Space Blue)
* **Knob 3**: Symmetry 1 (Pale Green)
* **Knob 4**: Fold 2 (Brick Ember)
* **Knob 5**: Offset 2 (Amber / Space Blue)
* **Knob 6**: Symmetry 2 (Pale Green)

## CV Inputs

The 6 CV inputs dynamically map to the Wave Folder parameters:

* **Jack 1**: Fold 1
* **Jack 2**: Fold 2
* **Jack 3**: Offset 1
* **Jack 4**: Offset 2
* **Jack 5**: Symmetry 1
* **Jack 6**: Symmetry 2

## Page 2: Feedback & Filter

The second page features effects applied globally to both channels (post-folder):

### Buttons
* **B1**: Change Page
* **B2**: Cycle Distortion Routing
  * Mode 1 (Off): Bypass (No distortion)
  * Mode 2 (Dim Orange): Pre-Filter Distortion
  * Mode 3 (Dim Brick Ember): Post-Filter Distortion
* **B3**: Cycle Filter Mode
  * Mode 1 (Dim Brick Ember): LowPass
  * Mode 2 (Dim Orange): BandPass
  * Mode 3 (Dim Spruce Blue): HighPass

### Knobs
* **Knob 1**: Feedback Amount (Purple)
* **Knob 2**: Distortion Amount (Orange)
* **Knob 3**: Feedback Delay Time (Purple)
* **Knob 4**: Distortion Bias / Symmetry (Orange)
* **Knob 5**: Filter Cutoff (Spruce Blue)
* **Knob 6**: Filter Resonance (Spruce Blue)

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
