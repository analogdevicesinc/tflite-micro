This folder contains the INT8 quantized model for urbansound classification.

> **Important — Weight reordering required:** The model data files (`.cc`/`.h`) in this folder must
> be generated from a **weight-reordered** INT8 TFLite model using `reorder_weights_fc_and_conv1x1.py`
> (located in `cces/Utils/data/scripts/`). Using a plain INT8 quantized model without reordering will
> produce **incorrect inference results** on the SHARC-FX target due to the DSP SIMD kernel layout requirements.
> Run the conversion script in `cces/Utils/automated-model-conversion/urbansound_classification/` to regenerate
> these files correctly.
