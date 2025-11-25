#!/bin/bash

project_folder="dtln"
env="python_env"
tflm_models="TFLM_models"

tflm_model1a="model_int16_1_data.h"
tflm_model1b="model_int16_1_data.cc"
tflm_model2a="model_int16_2_data.h"
tflm_model2b="model_int16_2_data.cc"
tflm_f_model1a="model_float_1_data.h"
tflm_f_model1b="model_float_1_data.cc"
tflm_f_model2a="model_float_2_data.h"
tflm_f_model2b="model_float_2_data.cc"

final_int16_model_path="../../../examples/denoiser/common/model/model_int16"
final_float_model_path="../../../examples/denoiser/common/model/model_fp"

# Check if models exist
if [[ -f "$tflm_models/$tflm_model1a" && -f "$tflm_models/$tflm_model1b" && -f "$tflm_models/$tflm_model2a" && -f "$tflm_models/$tflm_model2b" ]]; then
    echo "TFLM models have been generated successfully"
else
    if [[ ! -d "$project_folder" ]]; then
        git clone https://github.com/breizhn/DTLN.git "$project_folder"
    else
        echo "Project directory already present, moving to next step"
    fi

    cp scripts/DTLN_model.py "$project_folder/DTLN_model.py"
    cp scripts/input1.npy "$project_folder/input1.npy"
    cp scripts/input2.npy "$project_folder/input2.npy"
    cp scripts/state1.npy "$project_folder/state1.npy"
    cp scripts/state2.npy "$project_folder/state2.npy"
    cp scripts/fix_error.py "$project_folder/fix_error.py"

    cd "$project_folder" || exit

    if ! command -v python3.10 &> /dev/null; then
        echo "Python 3.10 not found. Installing..."
        apt-get update
        apt-get install -y lsb-release
        apt update
        apt install -y software-properties-common
        add-apt-repository -y ppa:deadsnakes/ppa
        apt update
        apt install -y python3.10 python3.10-venv python3.10-distutils python3-pip

    # (Insert install commands here)
    else
        echo "Python 3.10 is already installed."
    fi


    python3.10 -m pip install virtualenv
    python3.10 -m venv "$env"
    source "$env/bin/activate"

    pip install tensorflow==2.15.0 soundfile wavinfo

    python fix_error.py
    python convert_weights_to_tf_lite.py -m pretrained_model/model.h5 -t model_float
    python convert_weights_to_tf_lite.py -m pretrained_model/model.h5 -t model_int16 -q true

    cp model_int16_1.tflite ../scripts/
    cp model_int16_2.tflite ../scripts/
    cp model_float_1.tflite ../scripts/
    cp model_float_2.tflite ../scripts/

    cd ../scripts || exit
    if [[ ! -d "$tflm_models" ]]; then
        python generate_cc_arrays.py "$tflm_models" model_int16_2.tflite
        python generate_cc_arrays.py "$tflm_models" model_int16_1.tflite
        python generate_cc_arrays.py "$tflm_models" model_float_1.tflite
        python generate_cc_arrays.py "$tflm_models" model_float_2.tflite
    fi

    if [[ -d "../$tflm_models" ]]; then
        rm -rf "../$tflm_models"
    fi

    mv "$tflm_models" ../
    rm model_int16_1.tflite model_int16_2.tflite model_float_1.tflite model_float_2.tflite

    cd .. || exit
    cd "$project_folder" || exit

    echo "Reclaiming space..."
    rm -rf "$env"

    cd .. || exit
    rm -rf "$project_folder"

    if [[ -f "$tflm_models/$tflm_model1a" && -f "$tflm_models/$tflm_model1b" && -f "$tflm_models/$tflm_model2a" && -f "$tflm_models/$tflm_model2b" ]]; then
        echo "TFLM models have been generated successfully"
    fi
fi

# Copy float models to denoiser
cp "$tflm_models/$tflm_f_model1a" "$final_float_model_path/$tflm_f_model1a"
cp "$tflm_models/$tflm_f_model1b" "$final_float_model_path/$tflm_f_model1b"
cp "$tflm_models/$tflm_f_model2a" "$final_float_model_path/$tflm_f_model2a"
cp "$tflm_models/$tflm_f_model2b" "$final_float_model_path/$tflm_f_model2b"

if [[ -f "$final_float_model_path/$tflm_f_model1a" && -f "$final_float_model_path/$tflm_f_model1b" && -f "$final_float_model_path/$tflm_f_model2a" && -f "$final_float_model_path/$tflm_f_model2b" ]]; then
    echo "TFLM models have been copied into respective projects successfully, check the following paths to find the models:"
    echo "$final_float_model_path"
fi
