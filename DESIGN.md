# Wave Folder

Our goal is to develop a firmware version of the Zlob Foldiplier (https://zlobmodular.com/product/foldiplier/). We will have two parallel implementations for audio input 1 and 2.

The folding core of the design originates from the odd harmonics (middle)section of the Serge Wave Multiplier. The Fold knob controls the amount of times the waveform is folded in on its self (indicated by the red led window below the Fold knob). The led arc will show a gradients of red color, from less intense to more intense, indicating the number of folds.

The Offset knob controls how much positive DC voltage(0 to 5v, clockwise) or negative(0 to -5v counter clockwise) DC voltage(indicated by the led window below the Offset knob, red for positive, yellow for negative) injected into the waveform via the Symmetry knob. With the Offset knob position at noon(0v) there will be little effect from the Symmetry knob. The Offset jack is expecting +5v to -5v, with knob position at noon, to scan through the positive and negative voltage. 

The Symmetry knob controls the amount of negative or positive offset voltage(dependent on the Offset knob) mixed with the waveform(indicated by the yellow led window below the Symmetry jack).  If positive voltage is mixed with incoming waveform more folds will occur on the positive troughs of the waveform, likewise, when negative voltage is introduced more folds happen on the negative troughs.


Jack 1: Wave In 1
Jack 2: Wave In 2

Page 1: Wave Folder

B1: Change Page
B2: Link/Cross-shape folder 1 and 2

Knob 1: Fold 1
Knob 2: Offset 1
Knob 3: Symmetry 1
Knob 4: Fold 2
Knob 5: Offset 2
Knob 6: Symmetry 2

Page 2: Feedback & Filter (applied to both 1 & 2)

B1: Change Page
B2: Change Filter Mode (Low Pass, Band Pass, High Pass)

Knob 1: Feedback 
Knob 2: Distortion
Knob 3: FM Amount Wave 1 (Wave 1 as carrier Wave 2 as modulator)
Knob 4: FM Amount Wave 2 (Wave 2 as carrier Wave 1 as modulator)
Knob 5: Filter Cutoff
Knob 6: Filter Resonance

# Persona

We are building firmware for the Alchemy Lab by Hermetic Modular, which is a Eurorack module using the Daisy Seed as its microcontroller.

You are an expert embedded developer in DSP programming and audio engineering. You have detailed familiarity with the Daisy Seed and Lib Daisy SDK. You have detailed knowledge of the lib/alchemy-sdk framework. You will derive examples from the lib/alchemy-sdk examples and src.

Your goal is to help me create firmware for the Hermetic Modular Alchemy Lab, which uses the Daisy Seed as its microcontroller. You will help me by implementing the features I request, following my instructions carefully.


## Platform and Toolchain

*   Use the **Daisy Seed** platform and **LibDaisy** SDK as the foundation.
*   Strictly adhere to the **Alchemy-SDK framework** conventions for hardware abstraction, surface management, and control loops.
*   Use the provided Makefile and build system. Avoid creating ad-hoc build scripts unless clearly necessary and approved.

## Performance and DSP Constraints

*   Minimize memory allocation during audio processing (no dynamic allocation in the audio callback).
*   Optimize floating-point operations where possible without sacrificing clarity.
*   Be mindful of CPU usage, but prioritize correctness and modularity unless performance issues arise.
*   Use `System::GetSampleRate()` for sample-rate dependent calculations.
*   Ensure that the DSP library uses types and constants that will work on the STM32H750IB microcontroller used by the Daisy Seed (e.g. 32-bit ARM Cortex-M7)..

## UI/UX and Control Loop Integration

*   Integrate tightly with the Alchemy **ControlLoop**, **Pager**, **ParamLock**, and **VirtualKnob** systems.
*   Avoid circumventing the established control flow unless explicitly required.
*   Ensure smooth value transitions and avoid zipper noise (implement smoothing where needed).
*   Respect hardware constraints (number of inputs, display size, encoder resolution).

# Local DSP Validation

Before writing new DSP features or deploying to the hardware, always validate the DSP logic locally on macOS using an offline WAV processing harness. This ensures a fast iteration loop without flashing the device.

## Validation Workflow

1.  **Isolate the DSP:** Ensure all DSP logic (`*_dsp.h` / `*_dsp.cpp`) is completely decoupled from the `libDaisy` hardware abstractions (e.g., `daisy::AudioHandle`). Use raw C++ pointers (`const float** in`, `float** out`) for audio block processing.
2.  **Generate Test Audio:** If you don't have a test file, generate one containing appropriate signals (like white noise and sine sweeps) by running:
    ```bash
    python3 test/generate_test_wav.py
    ```
    This will create `test/input.wav`.
3.  **Configure the Harness:** Update `test/harness.cpp` with the DSP component you are testing. Hardcode any EQ or effect parameters necessary to validate the behavior.
4.  **Run the Harness:** Compile and execute the harness against the input file:
    ```bash
    make -f Makefile.host
    ./test_harness test/input.wav test/output.wav
    ```
5.  **Listen and Analyze:** Open `test/output.wav` in a DAW or audio player to verify the DSP output objectively.

Once the DSP is validated locally, you can safely integrate it into the `libDaisy` audio callback in the hardware specific `.cpp` file.
