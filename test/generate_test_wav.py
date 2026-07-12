import wave
import struct
import math
import os

SAMPLE_RATE = 48000
DURATION = 2.0  # seconds

def generate_wav(filename, freq1, freq2):
    num_samples = int(SAMPLE_RATE * DURATION)
    
    with wave.open(filename, 'w') as wav_file:
        # 2 channels, 2 bytes per sample (16-bit), sample rate
        wav_file.setparams((2, 2, SAMPLE_RATE, num_samples, 'NONE', 'not compressed'))
        
        for i in range(num_samples):
            t = i / SAMPLE_RATE
            
            # Left channel: Sine wave 1
            left_val = math.sin(2 * math.pi * freq1 * t)
            
            # Right channel: Sine wave 2
            right_val = math.sin(2 * math.pi * freq2 * t)
            
            # Convert to 16-bit PCM
            left_pcm = int(left_val * 32767.0)
            right_pcm = int(right_val * 32767.0)
            
            # Pack as little-endian 16-bit integers
            data = struct.pack('<hh', left_pcm, right_pcm)
            wav_file.writeframesraw(data)

if __name__ == '__main__':
    script_dir = os.path.dirname(os.path.abspath(__file__))
    output_path = os.path.join(script_dir, 'input.wav')
    print(f"Generating {output_path}...")
    generate_wav(output_path, 100.0, 250.0)
    print("Done.")
