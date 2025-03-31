import warnings
warnings.filterwarnings("ignore")
import sys
import numpy as np
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

MODELPATH = r"../music-genre-classification/model/net.pt"
data = np.load("data.npy")
data    = torch.FloatTensor(data).view(1, 1, 128, 128)


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
    
# def representative_dataset_gen():
    # for input_value in data_keras:
        # yield [np.expand_dims(input_value, axis=0).astype(np.float32)]

net         = genreNet()
net.load_state_dict(torch.load(MODELPATH, map_location='cpu'))
net.eval()


# Conversion
torch_input_enc = data  # Example input for encoder

keras_model = nobuco.pytorch_to_keras(
    net,
    args=[torch_input_enc], kwargs=None,
    inputs_channel_order=ChannelOrder.TENSORFLOW,
    outputs_channel_order=ChannelOrder.TENSORFLOW
)
# Check if preds and the output of keras_model are bitexact
data_keras = data.numpy().transpose(0, 2, 3, 1)

# Convert the model to TFLite with int8 quantization
converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
def representative_dataset_gen():
    for input_value in data_keras:
        yield [np.expand_dims(input_value, axis=0).astype(np.float32)]
converter.representative_dataset = representative_dataset_gen
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8
tflite_model_int8 = converter.convert()
with open("int8_genre_ID.tflite", "wb") as f:
    f.write(tflite_model_int8)

# Convert the model to TFLite with float32 format
converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)
# converter.optimizations = [tf.lite.Optimize.DEFAULT]
# converter.representative_dataset = representative_dataset_gen
tflite_model_float32 = converter.convert()
converter.inference_input_type = tf.float32
converter.inference_output_type = tf.float32
with open("float32_genre_ID.tflite", "wb") as f:
    f.write(tflite_model_float32)
