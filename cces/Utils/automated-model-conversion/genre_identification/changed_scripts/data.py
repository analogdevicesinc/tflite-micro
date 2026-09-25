import pandas as pd
import numpy as np
import pickle
import os

from librosa.core import load
from librosa.feature import melspectrogram
from librosa import power_to_db
import librosa 
from config import RAW_DATAPATH
N_FFT = 4096
HOP_LENGTH = 1024

class Data():

    def __init__(self, genres, datapath):
        self.raw_data   = None
        self.GENRES     = genres
        self.DATAPATH   = datapath
        print("\n-> Data() object is initialized.")
    
    def make_raw_data(self):
        records = list()
        mel_frames = [] # List to store mel frames
        print('self.GENRES',self.GENRES)
        for i, genre in enumerate(self.GENRES):
            print('genre',genre)
            GENREPATH = self.DATAPATH + genre + '/'
            for j, track in enumerate(os.listdir(GENREPATH)):
                TRACKPATH   = GENREPATH + track
                print("TRACKPATH", TRACKPATH)
                y, sr = load(TRACKPATH, sr=48000, mono=True)
                # Store STFT results
                stft_chunks = list()
                # Loop through the signal in steps of hop_length
                for i in range(0, len(y) - N_FFT + 1, HOP_LENGTH):
                    frame = y[i:i + N_FFT]
                    #print('frame shape',frame.shape)
                    D = librosa.stft(frame, n_fft=N_FFT, hop_length=HOP_LENGTH, center=False)
                    #print('D shape',D.shape) # D shape (2049, 1)             
                    # 2049 frequency bins (because n_fft // 2 + 1)
                    # 1 time frame (just one column)

                    S = np.abs(D)**2             
                    #print('S shape before',S.shape) #S shape before (2049, 1)
                    stft_chunks.append(S) 
              
                    if len(stft_chunks) == 128:  # If we have 128 frames, make a chunk
                          S_stack = np.hstack(stft_chunks)  
                          S_stack = S_stack / np.abs(np.max(S_stack) + 1e-9) # normalizing globally 
                          #print('S_stack shape after',S_stack.shape) #S shape after (2049, 128)
                          
                          mel_basis = librosa.filters.mel(sr=sr, n_fft=N_FFT, n_mels=128) #(2049, 128)
                          # Apply mel filter bank to the power spectrogram
                          mel_S = np.dot(mel_basis, S_stack)  # shape (128, 128)
     
                          # 128 mel bands
                          # 128 time frame
                          mel_frame = mel_S.T
                          #print('mel_frame shape',mel_frame.shape) #mel_frame shape (1, 128)
                          records.append((mel_frame, genre))  
                          stft_chunks = [] # Reset for next chunk
                               
        self.raw_data = pd.DataFrame(records, columns=['spectrogram', 'genre'])
        print("self.raw_data shape: ", self.raw_data.shape)
        return
    
    '''
    #48KHZ and stft normalized 
    def make_raw_data(self):
        records = list()
        for i, genre in enumerate(self.GENRES):
            GENREPATH = self.DATAPATH + genre + '/'
            for j, track in enumerate(os.listdir(GENREPATH)):
                TRACKPATH   = GENREPATH + track
                # print("%d.%s\t\t%s (%d)" % (i + 1, genre, TRACKPATH, j + 1))
                # changed by saurav
                #y, sr       = load(TRACKPATH, mono=True)
                y, sr = load(TRACKPATH, sr=48000, mono=True)
                #print('TRACKPATH',TRACKPATH)
                #print('y.shape',y.shape)
                
                #Way 1: 
                #S           = melspectrogram(y=y, sr=sr).T
                
                #Way 2: 
                #Step 1: STFT calculate                  
                D = librosa.stft(y, n_fft=4096, hop_length=1024) #1025, 2814,
                # Frequency bins = 2048/2+1 = 1025
                
                #sr = 48000
                #n_fft = 2048
                #hop_length = 512
                #Audio length: e.g. len(y) = 1440000 (30 seconds at 48kHz)
                #n_frames = int(np.ceil((len(y) + n_fft) / hop_length))
                #         = int(np.ceil((1440000 + 2048) / 512))
                #         = int(np.ceil(1442048 / 512))
                #         = 2815
                
                # Step 2: Convert power spectrogram
                S = np.abs(D)**2 # shape: (1025, n_frames)
                # is crucial because it converts the complex STFT output into a power spectrogram, which is the expected input   
                # for Mel filtering and later feature extraction.
                # [frequency bins, time axis]

                #Normalize S
                S = S/np.abs(np.max(S))
                
                ## Step 4: Apply random gain (e.g., between 0.3 and 1.5)
                # gain = np.random.uniform(0.3, 1.5)
                # S = S * gain
                 
                # Step 3: Create mel filter bank
                mel_basis = librosa.filters.mel(sr=sr, n_fft=4096, n_mels=128) # shape: (128, 1025)

                # Step 4: Apply mel filter bank
                mel_S = np.dot(mel_basis, S) # shape: (128, n_frames)
                S =mel_S.T
                #print('S',S)
                #print('S.shape before',S.shape)
                
                #print('S.shape before',S.shape)
                S           = S[:-1 * (S.shape[0] % 128)]
                #print('S.shape after',S.shape)
                num_chunk   = S.shape[0] / 128
                if num_chunk == 0:
                    continue 
                data_chunks = np.split(S, num_chunk)
                #print('len(data_chunks)',len(data_chunks))
                data_chunks = [(data, genre) for data in data_chunks]
                records.append(data_chunks)

        records = [data for record in records for data in record]
        self.raw_data = pd.DataFrame.from_records(records, columns=['spectrogram', 'genre'])
        return
    '''
    
    def save(self):
        with open(RAW_DATAPATH, 'wb') as outfile:
            pickle.dump(self.raw_data, outfile, pickle.HIGHEST_PROTOCOL)
        print('-> Data() object is saved.\n')
        return

    def load(self):
        with open(RAW_DATAPATH, 'rb') as infile:
            self.raw_data   = pickle.load(infile)
        print("-> Data() object is loaded.")
        return