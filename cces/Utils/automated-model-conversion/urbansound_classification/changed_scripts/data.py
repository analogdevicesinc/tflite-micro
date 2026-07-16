import pandas as pd
import numpy as np
import pickle
import os
from librosa.core import load
import librosa
from config import RAW_DATAPATH
import soundfile as sf
import matplotlib.pyplot as plt
from scipy.signal import lfilter, upfirdn
import logging
import random
# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)
# Set random seed for reproducible results
np.random.seed(42)


N_FFT = 4096
HOP_LENGTH = 1024
# Constants
MIN_STFT_CHUNKS = 89  # minimum number of STFT chunks to consider padding
NUM_STFT_CHUNKS = 128
NUM_MELS = 128

# Audio Augmentation Parameters
ENABLE_AUGMENTATION = True  # Set to False to disable augmentation
AUGMENTATION_PROBABILITY = 0.3  # 30% chance to apply augmentation to each audio file

# Noise parameters
GAUSSIAN_NOISE_STD = 0.005  # Standard deviation for Gaussian noise
UNIFORM_NOISE_SCALE = 0.01  # Scale for uniform noise

# Time stretching parameters
TIME_STRETCH_FACTORS = [0.8, 0.9, 1.1, 1.2]  # Speed change factors

# Pitch shift parameters (in semitones)
PITCH_SHIFT_STEPS = [-2, -1, 1, 2]

# Volume scaling parameters
VOLUME_SCALE_FACTORS = [0.7, 0.8, 1.2, 1.3]

# Advanced augmentation parameters
BACKGROUND_NOISE_LEVEL = 0.02
REVERB_STRENGTH = 0.1
FREQ_MASK_PARAM = 1000

def set_augmentation_config(enable=True, prob=0.3, noise_std=0.005):
    """
    Configure augmentation parameters.
    
    Args:
        enable: Enable/disable augmentation
        prob: Probability of applying augmentation
        noise_std: Standard deviation for Gaussian noise
    """
    global ENABLE_AUGMENTATION, AUGMENTATION_PROBABILITY, GAUSSIAN_NOISE_STD
    ENABLE_AUGMENTATION = enable
    AUGMENTATION_PROBABILITY = prob
    GAUSSIAN_NOISE_STD = noise_std
    
    logger.info(f"Augmentation config: enable={enable}, prob={prob}, noise_std={noise_std}")


# Removed decimation code - now working with 48kHz audio throughout


# -----------------------------------------------------------
# Audio Padding Function (Looped instead of zero padding)
# -----------------------------------------------------------
def pad_audio_with_looping(audio_frames, target_chunks=NUM_STFT_CHUNKS):
    """
    Pad STFT chunks with looped original audio instead of zeros.
    
    Args:
        audio_frames: Original audio signal (1D array)
        target_chunks: Target number of STFT chunks (default: 128)
        
    Returns:
        Padded audio that will generate exactly target_chunks STFT frames
    """
    current_length = len(audio_frames)
    
    if current_length == 0:
        logger.warning("Empty audio provided for padding")
        return audio_frames
    
    # Calculate required length for exact number of STFT chunks
    # Formula: (target_chunks - 1) * HOP_LENGTH + N_FFT
    required_length = (target_chunks - 1) * HOP_LENGTH + N_FFT
    
    if current_length >= required_length:
        # Audio is already long enough
        return audio_frames
    
    # Calculate padding needed
    padding_needed = required_length - current_length
    
    # Create padding by looping original audio
    full_loops = padding_needed // current_length
    remainder = padding_needed % current_length
    
    # Create looped padding
    padding_samples = np.concatenate([
        np.tile(audio_frames, full_loops),  # Full loops
        audio_frames[:remainder] if remainder > 0 else np.array([])  # Partial loop
    ])
    
    # Combine original with padding
    padded_audio = np.concatenate([audio_frames, padding_samples])
    
    logger.info(f"Looped padding: {current_length} -> {len(padded_audio)} samples "
                f"({padding_needed} samples added via looping)")
    
    return padded_audio.astype(np.float32)


# -----------------------------------------------------------
# Audio Augmentation Functions
# -----------------------------------------------------------
def add_gaussian_noise(audio, noise_std=GAUSSIAN_NOISE_STD):
    """Add Gaussian noise to audio signal."""
    noise = np.random.normal(0, noise_std, len(audio))
    return (audio + noise).astype(np.float32)


def add_uniform_noise(audio, noise_scale=UNIFORM_NOISE_SCALE):
    """Add uniform random noise to audio signal."""
    noise = np.random.uniform(-noise_scale, noise_scale, len(audio))
    return (audio + noise).astype(np.float32)


def time_stretch(audio, sr, stretch_factor):
    """Apply time stretching without changing pitch."""
    try:
        stretched = librosa.effects.time_stretch(audio, rate=stretch_factor)
        return stretched.astype(np.float32)
    except Exception as e:
        logger.warning(f"Time stretch failed: {e}, returning original audio")
        return audio


def pitch_shift(audio, sr, n_steps):
    """Shift pitch by n_steps semitones."""
    try:
        shifted = librosa.effects.pitch_shift(audio, sr=sr, n_steps=n_steps)
        return shifted.astype(np.float32)
    except Exception as e:
        logger.warning(f"Pitch shift failed: {e}, returning original audio")
        return audio


def add_background_noise(audio, noise_level=0.02):
    """Add colored background noise (pink noise simulation)."""
    # Generate pink-ish noise by filtering white noise
    white_noise = np.random.normal(0, 1, len(audio))
    
    # Simple pink noise approximation (1/f characteristics)
    # Apply moving average to create correlated noise
    window_size = max(1, len(audio) // 1000)
    if window_size > 1:
        kernel = np.ones(window_size) / window_size
        pink_noise = np.convolve(white_noise, kernel, mode='same')
    else:
        pink_noise = white_noise
    
    # Normalize and scale
    pink_noise = pink_noise / (np.std(pink_noise) + 1e-8) * noise_level
    return (audio + pink_noise).astype(np.float32)


def frequency_mask(audio, sr, num_masks=1, freq_mask_param=1000):
    """Apply frequency masking in the time domain (simulated)."""
    # This is a simplified version - creates notch-like effects
    try:
        # Create random frequency notches
        for _ in range(num_masks):
            # Generate a simple notch filter effect by modulating amplitude
            freq = np.random.uniform(100, sr//4)  # Random frequency to attenuate
            t = np.arange(len(audio)) / sr
            modulation = 1 - 0.3 * np.sin(2 * np.pi * freq * t)  # Gentle attenuation
            audio = audio * modulation
        return audio.astype(np.float32)
    except Exception as e:
        logger.warning(f"Frequency masking failed: {e}")
        return audio


def add_reverb_simulation(audio, sr, reverb_strength=0.1):
    """Add simple reverb simulation using delayed echoes."""
    try:
        # Simple echo-based reverb simulation
        delay_samples = int(0.05 * sr)  # 50ms delay
        if delay_samples < len(audio):
            echo = np.zeros_like(audio)
            echo[delay_samples:] = audio[:-delay_samples] * reverb_strength
            reverb_audio = audio + echo
            # Normalize to prevent clipping
            reverb_audio = reverb_audio / (np.max(np.abs(reverb_audio)) + 1e-8)
            return reverb_audio.astype(np.float32)
        else:
            return audio
    except Exception as e:
        logger.warning(f"Reverb simulation failed: {e}")
        return audio


def volume_scale(audio, scale_factor):
    """Scale audio volume by a factor."""
    scaled = audio * scale_factor
    # Clip to prevent overflow
    scaled = np.clip(scaled, -1.0, 1.0)
    return scaled.astype(np.float32)


def apply_audio_augmentation(audio, sr, augment_prob=AUGMENTATION_PROBABILITY):
    """
    Apply random audio augmentation with given probability.
    
    Args:
        audio: Input audio signal
        sr: Sample rate
        augment_prob: Probability of applying augmentation
    
    Returns:
        Augmented audio signal
    """
    if not ENABLE_AUGMENTATION or random.random() > augment_prob:
        return audio
    
    # Randomly choose augmentation type (weighted towards simpler augmentations)
    augmentation_types = [
        'gaussian_noise', 'gaussian_noise',  # Higher probability for noise
        'uniform_noise', 'background_noise',
        'time_stretch', 'pitch_shift', 'volume_scale',
        'frequency_mask', 'reverb_simulation'
    ]
    aug_type = random.choice(augmentation_types)
    
    try:
        if aug_type == 'gaussian_noise':
            return add_gaussian_noise(audio)
        elif aug_type == 'uniform_noise':
            return add_uniform_noise(audio)
        elif aug_type == 'background_noise':
            return add_background_noise(audio)
        elif aug_type == 'time_stretch':
            stretch_factor = random.choice(TIME_STRETCH_FACTORS)
            return time_stretch(audio, sr, stretch_factor)
        elif aug_type == 'pitch_shift':
            n_steps = random.choice(PITCH_SHIFT_STEPS)
            return pitch_shift(audio, sr, n_steps)
        elif aug_type == 'volume_scale':
            scale_factor = random.choice(VOLUME_SCALE_FACTORS)
            return volume_scale(audio, scale_factor)
        elif aug_type == 'frequency_mask':
            return frequency_mask(audio, sr)
        elif aug_type == 'reverb_simulation':
            return add_reverb_simulation(audio, sr)
        else:
            return audio
    except Exception as e:
        logger.warning(f"Augmentation {aug_type} failed: {e}, returning original audio")
        return audio



class Data():
    def __init__(self, genres, datapath):
        self.raw_data = None
        self.GENRES   = genres
        self.DATAPATH = datapath
        self.problem_files = []  # store info of audios not giving 128 chunks
        print("\n-> Data() object is initialized.")
    
    def make_raw_data(self):
        records = []
        df = pd.read_excel("../data/FilteredFreeSounds.xlsx")
        print("Dataframe shape: ", df.shape)

        audio_considered =0
        for idx, row in df.iterrows():
            #audio_considered = audio_considered + 1
            #continue 
            
            if 'License' in row and row['License'] == 'Attribution 3.0':
                continue

            fold = f"fold{row['fold']}"
            filename = row['slice_file_name']
            class_id = row['classID']
            class_names = row['class']
            file_path = os.path.join(self.DATAPATH, fold, filename)

            if not os.path.isfile(file_path):
                print(f"File not found: {file_path}")
                continue

            # Load original at 48kHz and keep it at 48kHz (no decimation)
            y, sr = load(file_path, sr=48000, mono=True)
            
            # Apply audio augmentation (only during training)
            if ENABLE_AUGMENTATION:
                y = apply_audio_augmentation(y, sr)
                logger.debug(f"Applied augmentation to {filename}")
            
            already_used = False
            duration_sec = len(y) / sr
            
            # For shorter files, apply looped padding to reach required length for 128 chunks
            if duration_sec < 5.461333333:  # 5.461333333 = (127*1024 + 4096)/48000 for 48kHz
                # Use looped padding instead of decimation for short files
                y = pad_audio_with_looping(y, NUM_STFT_CHUNKS)
                logger.info(f"Applied looped padding to short file {filename} ({class_names})")
            
            # No decimation - continue with 48kHz audio

            # Continue with feature extraction
            stft_chunks = []
            for i in range(0, len(y) - N_FFT + 1, HOP_LENGTH):
                frame = y[i:i + N_FFT]
                D = librosa.stft(frame, n_fft=N_FFT, hop_length=HOP_LENGTH, center=False)
                S = np.abs(D)**2
                stft_chunks.append(S)
                if len(stft_chunks) == NUM_STFT_CHUNKS:
                    S_stack = np.hstack(stft_chunks)
                    S_stack = S_stack / np.abs(np.max(S_stack) + 1e-9)

                    mel_basis = librosa.filters.mel(sr=sr, n_fft=N_FFT, n_mels=NUM_MELS)
                    mel_S = np.dot(mel_basis, S_stack)
                    mel_frame = mel_S.T

                    records.append((fold, class_id, class_names, mel_frame))
                    stft_chunks = []
                    already_used = True 

            n_chunks = len(stft_chunks)
            filename = os.path.basename(file_path)
            print('file, class_names, len(n_chunks)',filename,class_names,n_chunks)
          
            if class_names.lower() in ["car_horn", "gun_shot"]:
                # Always pad to 128 chunks using looped padding
                if n_chunks < NUM_STFT_CHUNKS:
                    if n_chunks > 0:
                        # Apply looped padding to the original audio to get exact 128 chunks
                        y_padded = pad_audio_with_looping(y, NUM_STFT_CHUNKS)
                        
                        # Recompute STFT with padded audio
                        stft_chunks_padded = []
                        for i in range(0, len(y_padded) - N_FFT + 1, HOP_LENGTH):
                            frame = y_padded[i:i + N_FFT]
                            D = librosa.stft(frame, n_fft=N_FFT, hop_length=HOP_LENGTH, center=False)
                            S = np.abs(D)**2
                            stft_chunks_padded.append(S)
                            if len(stft_chunks_padded) == NUM_STFT_CHUNKS:
                                break
                        
                        if len(stft_chunks_padded) == NUM_STFT_CHUNKS:
                            S_stack = np.hstack(stft_chunks_padded)
                            S_stack = S_stack / np.abs(np.max(S_stack) + 1e-9)

                            mel_basis = librosa.filters.mel(sr=sr, n_fft=N_FFT, n_mels=NUM_MELS)
                            mel_S = np.dot(mel_basis, S_stack)
                            mel_frame = mel_S.T

                            records.append((fold, class_id, class_names, mel_frame))
                            already_used = True
                            logger.info(f"[Special] Looped padding applied to {filename} ({class_names}): {n_chunks} -> 128 chunks")
                    else:
                        logger.warning(f"Cannot pad {filename} ({class_names}): no STFT chunks available.") 

            elif MIN_STFT_CHUNKS <= n_chunks < NUM_STFT_CHUNKS:
                # Apply looped padding instead of zero padding
                if n_chunks > 0:
                    # Apply looped padding to the original audio to get exact 128 chunks
                    y_padded = pad_audio_with_looping(y, NUM_STFT_CHUNKS)
                    
                    # Recompute STFT with padded audio
                    stft_chunks_padded = []
                    for i in range(0, len(y_padded) - N_FFT + 1, HOP_LENGTH):
                        frame = y_padded[i:i + N_FFT]
                        D = librosa.stft(frame, n_fft=N_FFT, hop_length=HOP_LENGTH, center=False)
                        S = np.abs(D)**2
                        stft_chunks_padded.append(S)
                        if len(stft_chunks_padded) == NUM_STFT_CHUNKS:
                            break
                    
                    if len(stft_chunks_padded) == NUM_STFT_CHUNKS:
                        S_stack = np.hstack(stft_chunks_padded)
                        S_stack = S_stack / np.abs(np.max(S_stack) + 1e-9)

                        mel_basis = librosa.filters.mel(sr=sr, n_fft=N_FFT, n_mels=NUM_MELS)
                        mel_S = np.dot(mel_basis, S_stack)
                        mel_frame = mel_S.T

                        records.append((fold, class_id, class_names, mel_frame))
                        already_used = True
                        logger.info(f"[General] Looped padding applied to {filename} ({class_names}): {n_chunks} -> 128 chunks")
                else:
                    logger.warning(f"Cannot pad {filename} ({class_names}): no STFT chunks available.") 

            elif n_chunks == 0:
                # Perfectly divisible into 128-blocks -> do nothing
                pass

            else:
                # Only log as problem if NO full 128-block was used earlier
                if not already_used:
                    self.problem_files.append({
                        "file": filename,
                        "class": class_names,
                        "fold": fold,
                        "chunks": n_chunks,
                        "sr": sr,
                        "length_samples": len(y),
                        "length_seconds": len(y) / sr
                    })
                # If already_used=True, just ignore the leftover tail
                continue
        
        self.raw_data = pd.DataFrame(records, columns=['fold', 'classID', 'class_names', 'spectrogram'])
        print(f"Raw data created with {len(self.raw_data)} records.")                
        print(f"{len(self.problem_files)} problem files stored in self.problem_files")

        # Dump problem files into CSV
        if self.problem_files:
            problem_df = pd.DataFrame(self.problem_files)
            problem_csv_path = "problem_files.csv"
            problem_df.to_csv(problem_csv_path, index=False)
            print(f"Problem files saved to {problem_csv_path} ({len(self.problem_files)} entries)")
        else:
            print("No problem files encountered.")

        return

    def save(self):
        with open(RAW_DATAPATH, 'wb') as outfile:
            pickle.dump(self.raw_data, outfile, pickle.HIGHEST_PROTOCOL)
        print('-> Data() object is saved.\n')
        return
        
    def load(self):
        with open(RAW_DATAPATH, 'rb') as infile:
            self.raw_data = pickle.load(infile)
        print("-> Data() object is loaded.")
        return  


"""
=============================================================================
AUDIO PROCESSING AND AUGMENTATION DOCUMENTATION
=============================================================================

This data.py file includes comprehensive audio processing and augmentation 
capabilities for robust neural network training. 

KEY FEATURES:

1. 48kHz AUDIO PROCESSING (No Decimation):
   - All audio is processed at original 48kHz sampling rate
   - No decimation to 24kHz is applied
   - Consistent sample rate throughout the pipeline

2. LOOPED PADDING (Instead of Zero Padding):
   - Short audio files are padded by looping the original audio content
   - Maintains natural audio characteristics instead of silence
   - Ensures exactly 128 STFT chunks for consistent mel spectrogram size

3. AUDIO AUGMENTATION TECHNIQUES:
   - Gaussian Noise: Adds white noise with configurable standard deviation
   - Uniform Noise: Adds uniform random noise
   - Background Noise: Simulates colored/pink noise for realistic background
   - Time Stretching: Changes playback speed without affecting pitch
   - Volume Scaling: Randomly scales audio amplitude
   - Pitch Shifting: Changes pitch without affecting tempo
   - Frequency Masking: Simulates frequency-selective attenuation
   - Reverb Simulation: Adds echo-based reverb effects

CONFIGURATION:
- Set ENABLE_AUGMENTATION = True/False to enable/disable
- AUGMENTATION_PROBABILITY controls how often augmentation is applied (0.3 = 30%)
- Individual parameters can be tuned for each augmentation type

PADDING BEHAVIOR:
- Files shorter than required length are padded with looped original content
- Special handling for car_horn and gun_shot classes (always padded)
- General padding applied to files with MIN_STFT_CHUNKS <= chunks < NUM_STFT_CHUNKS

USAGE:
The augmentation and padding are automatically applied during data loading 
in make_raw_data(). Each audio file has a 30% chance of being augmented 
with one random technique.

BENEFITS:
- 48kHz processing preserves all frequency information
- Looped padding creates more natural training data than silence
- Improved generalization to unseen acoustic conditions
- Better robustness to noise and distortions
- Reduced overfitting through data diversity

To disable augmentation (e.g., for testing), set ENABLE_AUGMENTATION = False
or use set_augmentation_config(enable=False)
=============================================================================
"""

