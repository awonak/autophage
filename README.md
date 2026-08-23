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

## Page 2: Feedback, Filter, and Distortion

The second page features effects applied globally to both channels (post-folder):

**Feedback**: The Feedback circuit feeds the output of the wave folder back into the input, creating complex, chaotic, or sustained tones. 
* **Feedback Amount** controls how much of the signal is fed back.
* **Feedback Delay Time** sets a short delay before the signal is fed back, allowing for comb-filtering and short metallic echoes.

**Distortion**: A Bazz Fuss-inspired gritty distortion adds aggressive hair and buzz to the signal.
* **Distortion Amount** controls the gain and intensity of the overdrive.
* **Distortion Bias** offsets the signal entering the distortion circuit, affecting the symmetry of the clipping and yielding different harmonic characteristics.
* **Button B2** lets you choose where the distortion sits in the chain: Off (bypassed), Pre-Filter, or Post-Filter.

**Multi-mode Filter**: A state-variable filter shapes the overall tone of your folded output.
* **Filter Cutoff** sweeps the cutoff frequency of the filter.
* **Filter Resonance** emphasizes the frequencies right at the cutoff point, adding a sharp, squelchy character.
* **Button B3** cycles through the filter modes: LowPass (cuts highs), BandPass (cuts highs and lows), and HighPass (cuts lows).


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
* **Knob 2**: Feedback Delay Time (Purple)
* **Knob 3**: Filter Cutoff (Spruce Blue)
* **Knob 4**: Distortion Amount (Orange)
* **Knob 5**: Distortion Bias / Symmetry (Orange)
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
