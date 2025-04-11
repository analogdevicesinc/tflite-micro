#!/bin/bash

# Variables
dtln="dtln"
env="python_env"
model1_h="model_float_1_data.h"
model1_cc="model_float_1_data.cc"
model2_h="model_float_2_data.h"
model2_cc="model_float_2_data.cc"

# Directories
model_dir="./TFLM_models"
example_model_dir="../../../examples/denoiser/common/model/model_fp"

# Check if models are already generated
if [[ -f "$model_dir/$model1_h" && -f "$model_dir/$model1_cc" && -f "$model_dir/$model2_h" && -f "$model_dir/$model2_cc" ]]; then
        echo "TFLM models have been generated successfully"
else
    # Clone the project if not present
    if [[ ! -d "$dtln" ]]; then
        git clone https://github.com/breizhn/DTLN.git "$dtln"
    else
        echo "Project directory already present, moving to next step"
    fi

    # Copy necessary files
    cp ./scripts/DTLN_model.py "$dtln/DTLN_model.py"
    cp ./scripts/input1.npy "$dtln/input1.npy"
    cp ./scripts/input2.npy "$dtln/input2.npy"
    cp ./scripts/state1.npy "$dtln/state1.npy"
    cp ./scripts/state2.npy "$dtln/state2.npy"
    cp ./scripts/fix_error.py "$dtln/fix_error.py"

    # Navigate to project folder
    cd "$dtln"

    # Set up Python environment
    python -m pip install --user virtualenv --no-cache-dir
    python -m venv "$env"
    source "$env/scripts/activate"
    python -m pip install --upgrade pip --no-cache-dir
    python -m pip install tensorflow==2.15.0 soundfile wavinfo --no-cache-dir

    # Run Python scripts
    python fix_error.py
    python convert_weights_to_tf_lite.py -m pretrained_model/model.h5 -t model_float

    # Copy generated TFLite models
    cp model_float_1.tflite ../scripts/
    cp model_float_2.tflite ../scripts/

    # Navigate to scripts folder
    cd ../scripts

    # Generate C arrays if TFLM models folder doesn't exist
    if [[ ! -d "../$model_dir" ]]; then
        python generate_cc_arrays.py . model_float_1.tflite
        python generate_cc_arrays.py . model_float_2.tflite
    fi

    # Clean up and move TFLM models
    mkdir ../$model_dir
    mv $model1_cc "../$model_dir/$model1_cc"
    mv $model1_h "../$model_dir/$model1_h"
    mv $model2_cc "../$model_dir/$model2_cc"
    mv $model2_h "../$model_dir/$model2_h"
    rm -f model_float_1.tflite model_float_2.tflite

    # Remove repository
    cd ..
    rm -rf "$dtln"

    # Check if TFLM models were generated successfully
    if [[ -f "$model_dir/$model1_h" && -f "$model_dir/$model1_cc" && -f "$model_dir/$model2_cc" && -f "$model_dir/$model1_h" ]]; then
        echo "TFLM models have been generated successfully"
    fi
fi

# Copy models to example project
cp "$model_dir/$model1_cc" "$example_model_dir/$model1_cc"
cp "$model_dir/$model1_h" "$example_model_dir/$model1_h"
cp "$model_dir/$model2_cc" "$example_model_dir/$model2_cc"
cp "$model_dir/$model2_h" "$example_model_dir/$model2_h"

# Verify copied models
if [[ -f "$example_model_dir/$model1_cc" && -f "$example_model_dir/$model1_h" && -f "$example_model_dir/$model2_cc" && -f "$example_model_dir/$model2_h" ]]; then
    echo "TFLM models have been copied into respective projects successfully, check the following paths to find the models:"
    echo "$example_model_dir"
fi