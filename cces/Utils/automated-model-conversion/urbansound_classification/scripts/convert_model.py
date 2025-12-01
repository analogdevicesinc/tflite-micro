import warnings
warnings.filterwarnings("ignore")
import sys
import numpy as np
import pickle
import nobuco
from nobuco import ChannelOrder, ChannelOrderingStrategy
from nobuco.layers.weight import WeightLayer
from nobuco.converters.node_converter import converter
from nobuco.converters.tensor import dim_pytorch2keras
from nobuco.converters.channel_ordering import set_channel_order, get_channel_order
from typing import Optional, Union, List, Tuple, Sequence, Any
import numbers
import keras
import torch
import torch.nn.functional as F
from torch import nn, Tensor
from torch.types import _int, _bool, Number, _dtype, _size
import tensorflow as tf

sys.path.insert(0, '../music-genre-classification/src')
from model import genreNet

# Paths
MODELPATH = "net.pt"
SET_DATAPATH = r"set.pkl"  # Path to your set.pkl file

# Load the set.pkl file containing train_set, valid_set, test_set
try:
    with open(SET_DATAPATH, "rb") as infile:
        train_set, valid_set, test_set = pickle.load(infile)
    print(f"Loaded training set with {len(train_set)} samples")
    print(f"Loaded validation set with {len(valid_set)} samples")
    print(f"Loaded test set with {len(test_set)} samples")
except Exception as e:
    print(f"Error loading set.pkl file: {e}")
    # Fallback to data.npy if set.pkl is not available
    print("Falling back to data.npy")
    data = np.load("data.npy")
    data = torch.FloatTensor(data).view(1, 1, 128, 128)
    train_set = None

# Extract spectrograms from train_set DataFrame
if train_set is not None:
    spectrograms = train_set['spectrogram'].values
    print(f"Extracted {len(spectrograms)} spectrograms from training set")
    # Use a sample of the data for representative dataset (first 100 or all if less than 100)
    # sample_size = min(100, len(spectrograms))
    sample_size = len(spectrograms)  # Use all for full testing
    representative_spectrograms = spectrograms[:sample_size]
    print(f"Using {sample_size} spectrograms for representative dataset")
else:
    # Fallback data
    representative_spectrograms = [data.numpy()[0, 0]]  # Use the single loaded spectrogram


@converter(nn.Conv2d)
def converter_Conv2d(self, input: Tensor):
    weight = self.weight
    bias = self.bias
    groups = self.groups
    padding = self.padding
    stride = self.stride
    dilation = self.dilation

    if isinstance(dilation, numbers.Number):
        dilation = (dilation, dilation)

    if isinstance(stride, numbers.Number):
        stride = (stride, stride)

    if isinstance(padding, numbers.Number):
        padding = (padding, padding)

    _, in_filters, _, _ = input.shape
    out_filters, _, kh, kw = weight.shape

    weights = weight.cpu().detach().numpy()
    weights = np.transpose(weights, (2, 3, 1, 0))

    if bias is not None:
        biases = bias.cpu().detach().numpy()
        params = [weights, biases]
        use_bias = True
    else:
        params = [weights]
        use_bias = False

    pad_str = 'same'
    pad_layer = None

    conv = keras.layers.Conv2D(
        filters=out_filters,
        kernel_size=(kh, kw),
        strides=stride,
        padding=pad_str,
        dilation_rate=dilation,
        groups=groups,
        use_bias=use_bias,
        weights=params
    )

    def func(input):
        if pad_layer is not None:
            input = pad_layer(input)
        output = conv(input)
        return output

    return func

@converter(F.log_softmax, torch.log_softmax, torch.Tensor.log_softmax, channel_ordering_strategy=ChannelOrderingStrategy.MINIMUM_TRANSPOSITIONS)
def converter_log_softmax(input: Tensor, dim: Optional[_int]=-1, *, dtype: Optional[_dtype]=None):
    num_dims = input.dim()

    def func(input, dim=-1, *, dtype=None):
        if get_channel_order(input) == ChannelOrder.TENSORFLOW:
            dim = dim_pytorch2keras(dim, num_dims)
        return tf.nn.softmax(input, axis=dim)
    return func

# Load the trained model
net = genreNet()
net.load_state_dict(torch.load(MODELPATH, map_location='cpu'))
net.eval()

# Create example input for conversion (use first spectrogram from training set or fallback data)
if train_set is not None:
    example_input = torch.FloatTensor(representative_spectrograms[0]).view(1, 1, 128, 128)
else:
    example_input = data

# Convert PyTorch model to Keras
torch_input_enc = example_input
keras_model = nobuco.pytorch_to_keras(
    net,
    args=[torch_input_enc], kwargs=None,
    inputs_channel_order=ChannelOrder.TENSORFLOW,
    outputs_channel_order=ChannelOrder.TENSORFLOW
)

print("PyTorch to Keras conversion completed")

# Define representative dataset generator using training data
def representative_dataset_gen():
    """
    Generator that yields representative input data for quantization.
    Uses spectrograms from the training set for better quantization accuracy.
    """
    for spectrogram in representative_spectrograms:
        # Reshape to match TensorFlow format: (batch_size, height, width, channels)
        # From (128, 128) to (1, 128, 128, 1)
        input_value = np.expand_dims(spectrogram, axis=(0, -1)).astype(np.float32)
        yield [input_value]

# Test the representative dataset generator
print("Testing representative dataset generator...")
sample_count = 0
for sample in representative_dataset_gen():
    sample_count += 1
    if sample_count == 1:
        print(f"Sample input shape: {sample[0].shape}")
        print(f"Sample input dtype: {sample[0].dtype}")
        print(f"Sample input range: [{np.min(sample[0]):.4f}, {np.max(sample[0]):.4f}]")
    if sample_count >= 5:  # Just test first 5 samples
        break
print(f"Representative dataset contains {sample_count} samples (showing first 5)")

# Convert the model to TFLite with int8 quantization
print("Converting to int8 quantized TFLite model...")
converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.representative_dataset = representative_dataset_gen
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

try:
    tflite_model_int8 = converter.convert()
    with open("int8_genre_ID.tflite", "wb") as f:
        f.write(tflite_model_int8)
    print("Int8 quantized model saved as 'int8_genre_ID.tflite'")
    print(f"Int8 model size: {len(tflite_model_int8)} bytes")
except Exception as e:
    print(f"Error during int8 quantization: {e}")

# Convert the model to TFLite with float32 format
print("Converting to float32 TFLite model...")
converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)
converter.inference_input_type = tf.float32
converter.inference_output_type = tf.float32

try:
    tflite_model_float32 = converter.convert()
    with open("float32_genre_ID.tflite", "wb") as f:
        f.write(tflite_model_float32)
    print("Float32 model saved as 'float32_genre_ID.tflite'")
    print(f"Float32 model size: {len(tflite_model_float32)} bytes")
except Exception as e:
    print(f"Error during float32 conversion: {e}")

# Convert the model to TFLite with float16 quantization
print("Converting to float16 quantized TFLite model...")
converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
converter.target_spec.supported_types = [tf.float16]

try:
    tflite_model_float16 = converter.convert()
    with open("float16_genre_ID.tflite", "wb") as f:
        f.write(tflite_model_float16)
    print("Float16 quantized model saved as 'float16_genre_ID.tflite'")
    print(f"Float16 model size: {len(tflite_model_float16)} bytes")
except Exception as e:
    print(f"Error during float16 quantization: {e}")

print("\nModel conversion completed!")
print("\nGenerated models:")
print("- int8_genre_ID.tflite (int8 quantized)")
print("- float32_genre_ID.tflite (float32)")
print("- float16_genre_ID.tflite (float16 quantized)")
