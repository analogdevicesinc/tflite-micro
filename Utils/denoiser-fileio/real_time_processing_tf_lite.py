"""
This is an example how to implement real time processing of the DTLN tf light
model in python.

Please change the name of the .wav file at line 43 before running the sript.
For .whl files of the tf light runtime go to: 
    https://www.tensorflow.org/lite/guide/python
    
Author: Nils L. Westhausen (nils.westhausen@uol.de)
Version: 30.06.2020

This code is licensed under the terms of the MIT-license.
"""

import soundfile as sf
import numpy as np
#import tflite_runtime.interpreter as tflite
import tensorflow as tf
import time
import os
import librosa

#set to true to use quantized int8 model
quant = True

if quant == False:
    model1_path = "no_norm_model_1.tflite"
    model2_path = "no_norm_model_2.tflite"
else:
    model1_path = "no_norm_model_int16_1.tflite"
    model2_path = "no_norm_model_int16_2.tflite"


##########################
# the values are fixed, if you need other values, you have to retrain.
# The sampling rate of 16k is also fix.
block_len = 512
block_shift = 128
# load models
interpreter_1 = tf.lite.Interpreter(model_path=model1_path)
interpreter_1.allocate_tensors()
interpreter_2 = tf.lite.Interpreter(model_path=model2_path)
interpreter_2.allocate_tensors()

# Get input and output tensors.
input_details_1 = interpreter_1.get_input_details()
output_details_1 = interpreter_1.get_output_details()

#tflm convertor messes up input order hence this jumbling 
#https://github.com/tensorflow/tensorflow/issues/33303
input_1_state_index = 0
input_1_mag_index = 1

input_details_2 = interpreter_2.get_input_details()
output_details_2 = interpreter_2.get_output_details()
output_2_state_index = 0
output_2_mag_index = 1

noise_folder='../tiny-denoiser/samples/dataset/noisy/'
output_folder='outputs'

#img1_datas = []
#img2_datas = []
for (dirpath, dirnames, filenames) in os.walk(noise_folder):
    for file in filenames:
        if file.endswith('.wav'):
            noisy_file = os.path.join(dirpath,file)
            output_file = os.path.join(output_folder,file)
            print(f"Running model on sample {noisy_file} and writing result to {output_file}")
            
            states_1 = np.zeros(input_details_1[input_1_state_index]['shape']).astype('float32')
            states_2 = np.zeros(input_details_2[1]['shape']).astype('float32')

            # load audio file at 16k fs
            audio,fs = sf.read(noisy_file)
            # check for sampling rate
            if fs != 16000:            
                audio = librosa.resample(audio, orig_sr=fs, target_sr=16000)
            # preallocate output audio
            out_file = np.zeros((len(audio)))
            # create buffer
            in_buffer = np.zeros((block_len)).astype('float32')
            out_buffer = np.zeros((block_len)).astype('float32')
            # calculate number of blocks
            num_blocks = (audio.shape[0] - (block_len-block_shift)) // block_shift
            time_array = []  

            # iterate over the number of blcoks  
            for idx in range(num_blocks):
                start_time = time.time()
                # shift values and write to buffer
                in_buffer[:-block_shift] = in_buffer[block_shift:]
                in_buffer[-block_shift:] = audio[idx*block_shift:(idx*block_shift)+block_shift]
                # calculate fft of input block
                in_block_fft = np.fft.rfft(in_buffer)
                in_mag = np.abs(in_block_fft)
                in_phase = np.angle(in_block_fft)
                # reshape magnitude to input dimensions
                in_mag = np.reshape(in_mag, (1,1,-1)).astype('float32')
                
                input_mag = in_mag#to store input for future use 
                
                input_type = input_details_1[input_1_mag_index]['dtype']
                if input_type == np.int8 or input_type == np.int16:
                    input_scale, input_zero_point = input_details_1[input_1_mag_index]['quantization']
                    #print("Input mag scale:", input_scale)
                    #print("Input mag zero point:", input_zero_point)
                    input_mag = (input_mag / input_scale) + input_zero_point
                    
                    # Convert features to NumPy array of expected type
                    input_mag = input_mag.astype(input_type)   

                    input_scale, input_zero_point = input_details_1[input_1_state_index]['quantization']
                    #print("Input state scale:", input_scale)
                    #print("Input state zero point:", input_zero_point)                   
                    states_1 = (states_1 / input_scale) + input_zero_point
                    # Convert features to NumPy array of expected type
                    states_1 = states_1.astype(input_type)                    
                
                # set tensors to the first model
                interpreter_1.set_tensor(input_details_1[input_1_state_index]['index'], states_1)
                interpreter_1.set_tensor(input_details_1[input_1_mag_index]['index'], input_mag)
                # run calculation 
                interpreter_1.invoke()
                # get the output of the first block
                out_mask = interpreter_1.get_tensor(output_details_1[0]['index']) 
                states_1 = interpreter_1.get_tensor(output_details_1[1]['index'])   
                                
                output_type = output_details_1[0]['dtype']
                if output_type == np.int8 or output_type == np.int16:
                    output_scale, output_zero_point = output_details_1[0]['quantization']
                    #print("Output mag scale:", output_scale)
                    #print("Output mag zero point:", output_zero_point)
                    #print()
                    out_mask = output_scale * (out_mask.astype(np.float32) - output_zero_point)
                    output_scale, output_zero_point = output_details_1[1]['quantization']
                    #print("Output state scale:", output_scale)
                    #print("Output state zero point:", output_zero_point)
                    states_1 = output_scale * (states_1.astype(np.float32) - output_zero_point)
               
                # calculate the ifft
                estimated_complex = in_mag * out_mask * np.exp(1j * in_phase)
                estimated_block = np.fft.irfft(estimated_complex)
                # reshape the time domain block
                estimated_block = np.reshape(estimated_block, (1,1,-1)).astype('float32')
               
                input_type = input_details_2[0]['dtype']
                if input_type == np.int8 or input_type == np.int16:
                    input_scale, input_zero_point = input_details_2[0]['quantization']
                    #print("Input mag scale:", input_scale)
                    #print("Input mag zero point:", input_zero_point)
                    estimated_block = (estimated_block / input_scale) + input_zero_point
                    
                    # Convert features to NumPy array of expected type
                    estimated_block = estimated_block.astype(input_type)
                    
                    input_scale, input_zero_point = input_details_2[1]['quantization']
                    #print("Input state scale:", input_scale)
                    #print("Input state zero point:", input_zero_point)
                    states_2 = (states_2 / input_scale) + input_zero_point
                    # Convert features to NumPy array of expected type
                    states_2 = states_2.astype(input_type)
                
                # set tensors to the second block
                interpreter_2.set_tensor(input_details_2[1]['index'], states_2)
                interpreter_2.set_tensor(input_details_2[0]['index'], estimated_block)
                # run calculation
                interpreter_2.invoke()
                # get output tensors
                out_block = interpreter_2.get_tensor(output_details_2[output_2_mag_index]['index']) 
                states_2 = interpreter_2.get_tensor(output_details_2[output_2_state_index]['index']) 
                
                output_type = output_details_2[output_2_mag_index]['dtype']
                if output_type == np.int8 or output_type == np.int16:
                    output_scale, output_zero_point = output_details_2[output_2_mag_index]['quantization']
                    #print("Output mag scale:", output_scale)
                    #print("Output mag zero point:", output_zero_point)
                    #print()
                    out_block = output_scale * (out_block.astype(np.float32) - output_zero_point)
                    output_scale, output_zero_point = output_details_2[output_2_state_index]['quantization']
                    #print("Output state scale:", output_scale)
                    #print("Output state zero point:", output_zero_point)
                    states_2 = output_scale * (states_2.astype(np.float32) - output_zero_point)
                                
                # shift values and write to buffer
                out_buffer[:-block_shift] = out_buffer[block_shift:]
                out_buffer[-block_shift:] = np.zeros((block_shift))
                out_buffer  += np.squeeze(out_block)
                # write block to output file
                out_file[idx*block_shift:(idx*block_shift)+block_shift] = out_buffer[:block_shift]
                                
                time_array.append(time.time()-start_time)
                
            # write to .wav file 
            sf.write(output_file, out_file, 16000) 
            print('Processing Time [ms]:')
            print(np.mean(np.stack(time_array))*1000)
            print('Processing finished.')
