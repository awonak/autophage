#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

// Include the DSP header we want to test
#include "autophage_dsp.h"

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input.wav> <output.wav>" << std::endl;
        return 1;
    }

    const char* inputFile = argv[1];
    const char* outputFile = argv[2];

    unsigned int channels;
    unsigned int sampleRate;
    drwav_uint64 totalPCMFrameCount;

    // Read input WAV
    float* pSampleData = drwav_open_file_and_read_pcm_frames_f32(inputFile, &channels, &sampleRate, &totalPCMFrameCount, NULL);
    if (pSampleData == NULL) {
        std::cerr << "Error reading " << inputFile << std::endl;
        return 1;
    }

    if (channels != 2) {
        std::cerr << "Expected 2 channels (stereo), but got " << channels << std::endl;
        drwav_free(pSampleData, NULL);
        return 1;
    }

    std::cout << "Loaded " << inputFile << ": " << totalPCMFrameCount << " frames, " << channels << " channels, " << sampleRate << " Hz" << std::endl;

    // Initialize DSP
    autophage_dsp::Init((float)sampleRate);

    // Hardcode some test parameters for the harness test
    autophage_dsp::ChannelParams params;
    params.fold = 0.0f;
    params.offset = 0.0f;
    params.symmetry = 0.0f;

    autophage_dsp::SetChannel(0, params);
    autophage_dsp::SetChannel(1, params);

    // Prepare arrays for the audio callback format
    std::vector<float> inL(totalPCMFrameCount);
    std::vector<float> inR(totalPCMFrameCount);
    std::vector<float> outL(totalPCMFrameCount);
    std::vector<float> outR(totalPCMFrameCount);

    for (size_t i = 0; i < totalPCMFrameCount; ++i) {
        inL[i] = pSampleData[i * 2 + 0];
        inR[i] = pSampleData[i * 2 + 1];
    }

    // Process block by block to allow parameter sweeping
    size_t blockSize = 32;
    for (size_t i = 0; i < totalPCMFrameCount; i += blockSize) {
        size_t frames = std::min(blockSize, (size_t)(totalPCMFrameCount - i));
        
        // Sweep fold from 0.0 to 1.0
        float progress = (float)i / (float)totalPCMFrameCount;
        params.fold = progress;
        // Give left channel some offset
        params.offset = 0.5f;
        params.symmetry = progress; 

        autophage_dsp::SetChannel(0, params);
        autophage_dsp::SetChannel(1, params);

        const float* in[2] = { inL.data() + i, inR.data() + i };
        float* out[2] = { outL.data() + i, outR.data() + i };
        
        autophage_dsp::Process(in, out, frames);
    }

    // Interleave output
    std::vector<float> outData(totalPCMFrameCount * 2);
    for (size_t i = 0; i < totalPCMFrameCount; ++i) {
        outData[i * 2 + 0] = outL[i];
        outData[i * 2 + 1] = outR[i];
    }

    // Write output WAV
    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
    format.channels = 2;
    format.sampleRate = sampleRate;
    format.bitsPerSample = 32;

    drwav wav;
    if (!drwav_init_file_write(&wav, outputFile, &format, NULL)) {
        std::cerr << "Error opening " << outputFile << " for writing" << std::endl;
        drwav_free(pSampleData, NULL);
        return 1;
    }

    drwav_write_pcm_frames(&wav, totalPCMFrameCount, outData.data());
    drwav_uninit(&wav);

    drwav_free(pSampleData, NULL);

    std::cout << "Wrote " << outputFile << std::endl;
    return 0;
}
