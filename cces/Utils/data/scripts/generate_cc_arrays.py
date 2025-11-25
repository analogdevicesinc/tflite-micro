# Copyright 2021 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
"""Library for converting .tflite, .bmp and .wav files to cc arrays."""

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import argparse
import os
import struct
import wave
import numpy as np

from PIL import Image


def generate_file(out_fname, array_name, array_type, array_contents, size):
  """Write an array of values to a CC or header file."""
  os.makedirs(os.path.dirname(out_fname), exist_ok=True)
  if out_fname.endswith('.cc'):
    out_cc_file = open(out_fname, 'w')
    out_cc_file.write('#include <cstdint>\n\n')
    out_cc_file.write('#include "{}"\n\n'.format(
        out_fname.split('genfiles/')[-1].replace('.cc', '.h')))
    out_cc_file.write('alignas(16) const {} {}[] = {{'.format(
        array_type, array_name))
    out_cc_file.write(array_contents)
    out_cc_file.write('};\n')
    out_cc_file.close()
  elif out_fname.endswith('.h'):
    out_hdr_file = open(out_fname, 'w')
    out_hdr_file.write('#include <cstdint>\n\n')
    out_hdr_file.write('constexpr unsigned int {}_size = {};\n'.format(
        array_name, str(size)))
    out_hdr_file.write('extern const {} {}[];\n'.format(
        array_type, array_name))
    out_hdr_file.close()
  else:
    raise ValueError('generated file must be end with .cc or .h')


def bytes_to_hexstring(buffer):
  """Convert a byte array to a hex string."""
  hex_values = [hex(buffer[i]) for i in range(len(buffer))]
  out_string = ','.join(hex_values)
  return out_string


def generate_array(input_fname):
  """Return array size and array of data from the input file."""
  if input_fname.endswith('.tflite'):
    with open(input_fname, 'rb') as input_file:
      buffer = input_file.read()
    size = len(buffer)
    out_string = bytes_to_hexstring(buffer)
    return [size, out_string]
  elif input_fname.endswith('.bmp'):
    img = Image.open(input_fname, mode='r')
    image_bytes = img.tobytes()
    size = len(image_bytes)
    out_string = bytes_to_hexstring(image_bytes)
    return [size, out_string]
  elif input_fname.endswith('.wav'):
    wav_file = wave.open(input_fname, mode='r')
    num_channels = wav_file.getnchannels()
    n_frames = wav_file.getnframes()
    frames = wav_file.readframes(n_frames)
    samples = struct.unpack('<%dh' % (num_channels * n_frames), frames)
    out_string = ','.join(map(str, samples))
    wav_file.close()
    return [wav_file.getnframes(), out_string]
  elif input_fname.endswith('.csv'):
    with open(input_fname, 'r') as input_file:
      # Assume one array per csv file.
      elements = input_file.readline()
      return [len(elements.split(',')), elements]
  elif input_fname.endswith('.npy'):
    data = np.float32(np.load(input_fname, allow_pickle=False))
    data_1d = data.flatten()
    out_string = ','.join([str(x) for x in data_1d])
    return [len(data_1d), out_string]

  else:
    raise ValueError('input file must be .tflite, .bmp, .wav or .csv')


def get_array_name(input_fname):
    """Returns a fixed name for the generated array when processing .wav files."""
    if input_fname.endswith('.wav'):
        return ["g_micro_speech_audio_data", "int16_t"]  # Fixed name for .wav
    elif input_fname.endswith('.tflite'):
        return ["g_model_data", "unsigned char"]
    elif input_fname.endswith('.bmp'):
        return ["g_image_data", "unsigned char"]
    elif input_fname.endswith('_int32.csv'):
        return ["g_test_data", "int32_t"]
    elif input_fname.endswith('_int16.csv'):
        return ["g_test_data", "int16_t"]
    elif input_fname.endswith('_int8.csv'):
        return ["g_test_data", "int8_t"]
    elif input_fname.endswith('_float.csv'):
        return ["g_test_data", "float"]
    elif input_fname.endswith('.npy'):
        return ["g_test_data", "float"]
    else:
        raise ValueError("Unsupported file type.")


def main():
    """Create cc sources with C arrays, always using 'micro_speech_audio_data' as the filename."""
    parser = argparse.ArgumentParser()
    parser.add_argument(
        'output',
        help='Base directory for all outputs or a specific .cc or .h file to generate.')
    parser.add_argument(
        'inputs',
        nargs='+',
        help='Input WAV, BMP, or TFLite files to convert. '
             'If output is a .cc or .h file, only one input may be specified.')
    args = parser.parse_args()

    # If the output is a specific .cc or .h file, process only one input
    if args.output.endswith('.cc') or args.output.endswith('.h'):
        assert len(args.inputs) == 1
        size, cc_array = generate_array(args.inputs[0])
        generated_array_name, array_type = get_array_name(args.inputs[0])
        generate_file(args.output, generated_array_name, array_type, cc_array, size)
    else:
        # Ensure the output directory exists
        os.makedirs(args.output, exist_ok=True)

        # Use a fixed filename for output
        output_base_fname = os.path.join(args.output, 'micro_speech_audio_data')

        output_cc_fname = output_base_fname + '.cc'
        output_hdr_fname = output_base_fname + '.h'

        # Print output CC filename for Make to include it in the build
        print(output_cc_fname)

        size, cc_array = generate_array(args.inputs[0])  # Process only the first input
        generated_array_name, array_type = get_array_name(args.inputs[0])
        generate_file(output_cc_fname, generated_array_name, array_type, cc_array, size)
        generate_file(output_hdr_fname, generated_array_name, array_type, cc_array, size)

if __name__ == "__main__":
    main()