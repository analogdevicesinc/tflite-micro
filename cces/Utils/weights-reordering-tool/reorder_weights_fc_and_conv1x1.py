#!/usr/bin/env python3
# Reorder FC + Conv1x1 weights to blocked layout for DSP SIMD access.

import sys
import argparse
import numpy as np
from pathlib import Path

try:
    from tensorflow.lite.python import schema_py_generated as schema_fb
except ImportError:
    print("Error: TensorFlow is required. Install with: pip install tensorflow")
    sys.exit(1)


# TFLite builtin operator codes
TFLITE_FULLY_CONNECTED = 9
TFLITE_CONV_2D = 3

TENSOR_TYPE_NUMPY = {
    0: np.float32, 1: np.float16, 2: np.int32, 3: np.uint8,
    4: np.int64,   7: np.int16,   9: np.int8,  10: np.float64,
    15: np.uint32, 16: np.uint16,
}

TENSOR_TYPE_NAMES = {
    0: "FLOAT32", 1: "FLOAT16", 2: "INT32", 3: "UINT8",
    4: "INT64",   7: "INT16",   9: "INT8",  10: "FLOAT64",
    15: "UINT32", 16: "UINT16",
}


class TFLiteWeightReorderer:

    def __init__(self, model_path: str):
        self.model_path = model_path

        with open(model_path, "rb") as f:
            self.buffer = bytearray(f.read())

        self.model = schema_fb.Model.GetRootAsModel(bytes(self.buffer), 0)
        self._parse_op_codes()
        self._find_fc_layers()
        self._find_conv1x1_layers()

    def _parse_op_codes(self):
        self.op_codes = []
        for i in range(self.model.OperatorCodesLength()):
            opc = self.model.OperatorCodes(i)
            code = opc.DeprecatedBuiltinCode()
            if code == 127:
                code = opc.BuiltinCode()
            self.op_codes.append(code)

    def _find_fc_layers(self):
        self.fc_layers = []
        for sg_idx in range(self.model.SubgraphsLength()):
            subgraph = self.model.Subgraphs(sg_idx)
            for op_idx in range(subgraph.OperatorsLength()):
                operator = subgraph.Operators(op_idx)
                opcode_idx = operator.OpcodeIndex()
                if opcode_idx >= len(self.op_codes):
                    continue
                if self.op_codes[opcode_idx] != TFLITE_FULLY_CONNECTED:
                    continue

                weight_idx = operator.Inputs(1)
                weight_tensor = subgraph.Tensors(weight_idx)
                weight_shape = [weight_tensor.Shape(i) for i in range(weight_tensor.ShapeLength())]

                input_tensor = subgraph.Tensors(operator.Inputs(0))
                output_tensor = subgraph.Tensors(operator.Outputs(0))
                input_shape = [input_tensor.Shape(i) for i in range(input_tensor.ShapeLength())]
                output_shape = [output_tensor.Shape(i) for i in range(output_tensor.ShapeLength())]

                bias_idx = operator.Inputs(2) if operator.InputsLength() > 2 else -1
                bias_tensor = subgraph.Tensors(bias_idx) if bias_idx >= 0 else None
                bias_shape = ([bias_tensor.Shape(i) for i in range(bias_tensor.ShapeLength())]
                              if bias_tensor else None)

                name = (weight_tensor.Name().decode("utf-8")
                        if weight_tensor.Name() else f"fc_weight_{op_idx}")

                quant = weight_tensor.Quantization()
                quantized = quant is not None and quant.ScaleLength() > 0

                self.fc_layers.append({
                    "kind":              "FC",
                    "subgraph_idx":      sg_idx,
                    "op_idx":            op_idx,
                    "name":              name,
                    "weight_tensor_idx": weight_idx,
                    "weight_buffer_idx": weight_tensor.Buffer(),
                    "weight_type":       weight_tensor.Type(),
                    "weight_shape":      weight_shape,
                    "input_shape":       input_shape,
                    "output_shape":      output_shape,
                    "bias_shape":        bias_shape,
                    "quantized":         quantized,
                })

    def _find_conv1x1_layers(self):
        self.conv1x1_layers = []
        for sg_idx in range(self.model.SubgraphsLength()):
            subgraph = self.model.Subgraphs(sg_idx)
            for op_idx in range(subgraph.OperatorsLength()):
                operator = subgraph.Operators(op_idx)
                opcode_idx = operator.OpcodeIndex()
                if opcode_idx >= len(self.op_codes):
                    continue
                if self.op_codes[opcode_idx] != TFLITE_CONV_2D:
                    continue

                weight_idx = operator.Inputs(1)
                weight_tensor = subgraph.Tensors(weight_idx)
                weight_shape = [weight_tensor.Shape(i) for i in range(weight_tensor.ShapeLength())]

                if len(weight_shape) != 4:
                    continue
                out_ch, kH, kW, in_ch = weight_shape
                if kH != 1 or kW != 1:
                    continue

                input_tensor = subgraph.Tensors(operator.Inputs(0))
                output_tensor = subgraph.Tensors(operator.Outputs(0))
                input_shape = [input_tensor.Shape(i) for i in range(input_tensor.ShapeLength())]
                output_shape = [output_tensor.Shape(i) for i in range(output_tensor.ShapeLength())]

                name = (weight_tensor.Name().decode("utf-8")
                        if weight_tensor.Name() else f"conv1x1_weight_{op_idx}")

                quant = weight_tensor.Quantization()
                quantized = quant is not None and quant.ScaleLength() > 0

                self.conv1x1_layers.append({
                    "kind":              "CONV1x1",
                    "subgraph_idx":      sg_idx,
                    "op_idx":            op_idx,
                    "name":              name,
                    "weight_tensor_idx": weight_idx,
                    "weight_buffer_idx": weight_tensor.Buffer(),
                    "weight_type":       weight_tensor.Type(),
                    "weight_shape":      weight_shape,
                    "input_shape":       input_shape,
                    "output_shape":      output_shape,
                    "out_ch":            out_ch,
                    "in_ch":             in_ch,
                    "quantized":         quantized,
                })

    def analyze(self):
        print("=" * 80)
        print("  WEIGHT LAYOUT ANALYSIS  (FC + Conv1x1)")
        print("=" * 80)
        print(f"  Model   : {self.model_path}")
        print(f"  Version : {self.model.Version()}")
        print(f"  Buffers : {self.model.BuffersLength()}")
        print()

        print(f"  FULLY_CONNECTED layers: {len(self.fc_layers)}")
        print("  " + "-" * 76)
        for i, fc in enumerate(self.fc_layers):
            buf = self.model.Buffers(fc["weight_buffer_idx"])
            w = fc["weight_shape"]
            params = w[0] * w[1] if len(w) == 2 else "?"
            print(f"  [{i+1:3d}]  op={fc['op_idx']:>3}  weight={w}  "
                  f"({TENSOR_TYPE_NAMES.get(fc['weight_type'], '?')})  "
                  f"params={params}  buf={buf.DataLength():,}B  "
                  f"quant={'Y' if fc['quantized'] else 'N'}  "
                  f"{fc['name']}")
        print()

        print(f"  CONV_2D 1x1 layers: {len(self.conv1x1_layers)}")

        print("  " + "-" * 76)
        for i, c in enumerate(self.conv1x1_layers):
            buf = self.model.Buffers(c["weight_buffer_idx"])
            print(f"  [{i+1:3d}]  op={c['op_idx']:>3}  weight={c['weight_shape']}  "
                  f"({TENSOR_TYPE_NAMES.get(c['weight_type'], '?')})  "
                  f"in_ch={c['in_ch']}  out_ch={c['out_ch']}  "
                  f"buf={buf.DataLength():,}B  "
                  f"quant={'Y' if c['quantized'] else 'N'}  "
                  f"{c['name']}")
        print()
        print("=" * 80)

    @staticmethod
    def _reorder_2d_blocked(weights_2d, block_size, dtype):
        # Transpose [out,in]->[in,out] then block into [out/block, in, block]
        out_size, in_size = weights_2d.shape

        weights_t = weights_2d.T.copy()

        num_blocks = out_size // block_size
        remainder = out_size % block_size

        if remainder == 0:
            blocked = np.zeros((num_blocks, in_size, block_size), dtype=dtype)
            for b in range(num_blocks):
                blocked[b, :, :] = weights_t[:, b * block_size:(b + 1) * block_size]
            blocked_bytes = blocked.reshape(-1).tobytes()
            shape_str = f"[{num_blocks}, {in_size}, {block_size}]"
        else:
            parts = []
            for b in range(num_blocks):
                parts.append(weights_t[:, b * block_size:(b + 1) * block_size].tobytes())
            tail = weights_t[:, num_blocks * block_size:].copy()
            parts.append(tail.tobytes())
            blocked_bytes = b"".join(parts)
            shape_str = (f"[{num_blocks}, {in_size}, {block_size}] + "
                         f"[{in_size}, {remainder}] tail")

        info = {
            "transposed_shape": (in_size, out_size),
            "blocked_shape_str": shape_str,
            "num_blocks": num_blocks,
            "remainder": remainder,
        }
        return blocked_bytes, info

    def reorder(self, output_path: str, block_size: int = 16,
                do_fc: bool = True, do_conv: bool = True,
                fc_filter_shape: tuple = None,
                verbose: bool = True):

        modified_buffer = bytearray(self.buffer)
        fc_results = []
        conv_results = []

        if do_fc:
            if verbose:
                print("=" * 70)
                print("  FC LAYERS")
                print("=" * 70)
            for fc in self.fc_layers:
                result = {"name": fc["name"], "shape": fc["weight_shape"]}
                buffer_idx = fc["weight_buffer_idx"]
                buf = self.model.Buffers(buffer_idx)

                if buf.DataLength() == 0:
                    result.update(status="skipped", reason="empty buffer")
                    fc_results.append(result)
                    if verbose:
                        print(f"  SKIP  {fc['name']}: empty buffer")
                    continue

                if fc_filter_shape is not None and tuple(fc["weight_shape"]) != fc_filter_shape:
                    result.update(status="skipped",
                                  reason=f"shape {fc['weight_shape']} != filter {list(fc_filter_shape)}")
                    fc_results.append(result)
                    if verbose:
                        print(f"  SKIP  {fc['name']}: shape {fc['weight_shape']} != filter {list(fc_filter_shape)}")
                    continue

                data_bytes = bytes([buf.Data(i) for i in range(buf.DataLength())])
                dtype = TENSOR_TYPE_NUMPY.get(fc["weight_type"], np.float32)

                try:
                    weights = np.frombuffer(data_bytes, dtype=dtype).reshape(fc["weight_shape"])
                except Exception as e:
                    result.update(status="skipped", reason=f"reshape error: {e}")
                    fc_results.append(result)
                    if verbose:
                        print(f"  SKIP  {fc['name']}: {e}")
                    continue

                if len(weights.shape) != 2:
                    result.update(status="skipped", reason=f"not 2D ({weights.shape})")
                    fc_results.append(result)
                    if verbose:
                        print(f"  SKIP  {fc['name']}: not 2D ({weights.shape})")
                    continue

                blocked_bytes, info = self._reorder_2d_blocked(weights, block_size, dtype)

                start_pos = modified_buffer.find(data_bytes)
                if start_pos == -1:
                    result.update(status="skipped", reason="could not locate data in buffer")
                    fc_results.append(result)
                    if verbose:
                        print(f"  SKIP  {fc['name']}: could not locate data in buffer")
                    continue

                modified_buffer[start_pos:start_pos + len(blocked_bytes)] = blocked_bytes
                result.update(status="reordered",
                              reason=f"blocked {info['blocked_shape_str']}, stride {block_size}")
                fc_results.append(result)
                if verbose:
                    out_f, in_f = fc["weight_shape"]
                    print(f"  OK    {fc['name']}")
                    print(f"        [{out_f}, {in_f}] -> transpose -> [{in_f}, {out_f}] -> {info['blocked_shape_str']}")

        if do_conv:
            if verbose:
                print()
                print("=" * 70)
                print("  CONV_2D 1x1 LAYERS")
                print("=" * 70)
            for c in self.conv1x1_layers:
                result = {"name": c["name"], "shape": c["weight_shape"]}
                buffer_idx = c["weight_buffer_idx"]
                buf = self.model.Buffers(buffer_idx)

                if buf.DataLength() == 0:
                    result.update(status="skipped", reason="empty buffer")
                    conv_results.append(result)
                    if verbose:
                        print(f"  SKIP  {c['name']}: empty buffer")
                    continue

                data_bytes = bytes([buf.Data(i) for i in range(buf.DataLength())])
                dtype = TENSOR_TYPE_NUMPY.get(c["weight_type"], np.float32)

                try:
                    weights = np.frombuffer(data_bytes, dtype=dtype).reshape(c["weight_shape"])
                except Exception as e:
                    result.update(status="skipped", reason=f"reshape error: {e}")
                    conv_results.append(result)
                    if verbose:
                        print(f"  SKIP  {c['name']}: {e}")
                    continue

                out_ch, _, _, in_ch = c["weight_shape"]

                weights_2d = weights.reshape(out_ch, in_ch)
                blocked_bytes, info = self._reorder_2d_blocked(weights_2d, block_size, dtype)

                start_pos = modified_buffer.find(data_bytes)
                if start_pos == -1:
                    result.update(status="skipped", reason="could not locate data in buffer")
                    conv_results.append(result)
                    if verbose:
                        print(f"  SKIP  {c['name']}: could not locate data in buffer")
                    continue

                modified_buffer[start_pos:start_pos + len(blocked_bytes)] = blocked_bytes
                result.update(status="reordered",
                              reason=f"blocked {info['blocked_shape_str']}, stride {block_size}")
                conv_results.append(result)
                if verbose:
                    print(f"  OK    {c['name']}")
                    print(f"        {c['weight_shape']} -> squeeze [{out_ch}, {in_ch}] "
                          f"-> transpose [{in_ch}, {out_ch}] -> {info['blocked_shape_str']}")

        with open(output_path, "wb") as f:
            f.write(modified_buffer)

        self._print_summary(fc_results, conv_results, block_size, output_path)

        return fc_results, conv_results

    @staticmethod
    def _print_summary(fc_results, conv_results, block_size, output_path):
        fc_ok = [r for r in fc_results if r["status"] == "reordered"]
        fc_skip = [r for r in fc_results if r["status"] == "skipped"]
        conv_ok = [r for r in conv_results if r["status"] == "reordered"]
        conv_skip = [r for r in conv_results if r["status"] == "skipped"]

        print()
        print("=" * 70)
        print("  SUMMARY")
        print("=" * 70)
        print(f"  Block size : {block_size}")
        print(f"  Output     : {output_path}")
        print()

        print(f"  FC layers: {len(fc_ok)} reordered, {len(fc_skip)} skipped "
              f"(of {len(fc_results)} total)")
        if fc_ok:
            for r in fc_ok:
                print(f"    [OK]   {r['name']}  {r['shape']}  ->  {r['reason']}")
        if fc_skip:
            for r in fc_skip:
                print(f"    [SKIP] {r['name']}  {r['shape']}  —  {r['reason']}")

        print()

        print(f"  Conv1x1 layers: {len(conv_ok)} reordered, {len(conv_skip)} skipped "
              f"(of {len(conv_results)} total)")
        if conv_ok:
            for r in conv_ok:
                print(f"    [OK]   {r['name']}  {r['shape']}  ->  {r['reason']}")
        if conv_skip:
            for r in conv_skip:
                print(f"    [SKIP] {r['name']}  {r['shape']}  —  {r['reason']}")

        print()
        total_ok = len(fc_ok) + len(conv_ok)
        total_skip = len(fc_skip) + len(conv_skip)
        total = len(fc_results) + len(conv_results)
        print(f"  TOTAL: {total_ok} reordered, {total_skip} skipped (of {total} layers)")
        print("=" * 70)


def main():
    parser = argparse.ArgumentParser(
        description="Reorder FC and Conv1x1 weights to blocked layout for DSP kernel",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("input", help="Input TFLite model path")
    parser.add_argument("output", nargs="?", default=None,
                        help="Output TFLite model path")
    parser.add_argument("--analyze", "-a", action="store_true",
                        help="Analyze only (no reordering)")
    parser.add_argument("--block", type=int, default=16, metavar="SIZE",
                        help="Block size (default: 16)")
    parser.add_argument("--fc-only", action="store_true",
                        help="Reorder only FC layers")
    parser.add_argument("--conv-only", action="store_true",
                        help="Reorder only conv1x1 layers")
    parser.add_argument("--fc-shape", type=str, metavar="SHAPE",
                        help="Only process FC layers matching this shape, e.g. '512,512'")
    parser.add_argument("--quiet", "-q", action="store_true",
                        help="Summary only")

    args = parser.parse_args()

    if not Path(args.input).is_file():
        print(f"Error: file not found: {args.input}")
        return 1

    fc_filter_shape = None
    if args.fc_shape:
        try:
            fc_filter_shape = tuple(int(x.strip()) for x in args.fc_shape.split(","))
            if len(fc_filter_shape) != 2:
                print(f"Error: --fc-shape must have 2 dimensions, got {len(fc_filter_shape)}")
                return 1
        except ValueError:
            print(f"Error: invalid shape format '{args.fc_shape}'. Use e.g. 512,512")
            return 1

    tool = TFLiteWeightReorderer(args.input)

    if args.analyze:
        tool.analyze()
        return 0

    if args.output is None:
        tool.analyze()
        return 0

    if args.fc_only and args.conv_only:
        print("Error: --fc-only and --conv-only are mutually exclusive")
        return 1

    do_fc = not args.conv_only
    do_conv = not args.fc_only

    tool.reorder(
        args.output,
        block_size=args.block,
        do_fc=do_fc,
        do_conv=do_conv,
        fc_filter_shape=fc_filter_shape,
        verbose=not args.quiet,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main() or 0)
