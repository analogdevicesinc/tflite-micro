#!/bin/bash

# Variables
project_folder="music-genre-classification"
env="python_env"
tflite_model_float32="float32_genre_ID.tflite"
tflite_model_int8="int8_genre_ID.tflite"
tflm_model1a="float32_genre_ID_model_data.h"
tflm_model1b="float32_genre_ID_model_data.cc"
tflm_model2a="int8_genre_ID_model_data.h"
tflm_model2b="int8_genre_ID_model_data.cc"

# Directories
tflm_models="TFLM_models"
final_int8_model_path="../../../examples/genre_identification/common/model/int8_genre_ID/"
final_float_model_path="../../../examples/genre_identification/common/model/float_genre_ID/"

# Check if TFLM models exist
if [[ -f "$tflm_models/$tflm_model1a" && -f "$tflm_models/$tflm_model1b" && -f "$tflm_models/$tflm_model2a" && -f "$tflm_models/$tflm_model2b" ]]; then
    echo "TFLM models have been generated successfully"
else
    # Clone the project if not present
    if [[ ! -d "$project_folder" ]]; then
        git clone https://github.com/cetinsamet/music-genre-classification.git "$project_folder"
    else
        echo "Project directory already present, moving to next step"
    fi

    # Navigate to scripts folder
    cd "scripts"

    # Set up Python environment
    if [[ ! -d "$env" ]]; then
        python -m pip install --user virtualenv --no-cache-dir
        python -m venv "$env"
        source "$env/scripts/activate"
        python -m pip install --upgrade pip --no-cache-dir
        cp requirements.txt reqs.txt
        sed -i '/tensorflow-intel==2.15.0/d' reqs.txt
        python -m pip install -r reqs.txt --no-cache-dir
        rm reqs.txt
    fi

    # Generate TFLite models if not already present
    if [[ ! -f "$tflite_model_int8" ]]; then
        source "$env/scripts/activate"
        python convert_model.py
    fi

    # Generate TFLM models if not already present
    if [[ ! -d "$tflm_models" ]]; then
        python generate_cc_arrays.py . "$tflite_model_float32"
        python generate_cc_arrays.py . "$tflite_model_int8"
    fi

    # Clean up and move TFLM models
    mkdir ../$tflm_models
    mv $tflm_model1a ../$tflm_models/$tflm_model1a
    mv $tflm_model1b ../$tflm_models/$tflm_model1b
    mv $tflm_model2a ../$tflm_models/$tflm_model2a
    mv $tflm_model2b ../$tflm_models/$tflm_model2b
    rm -f "$tflite_model_float32" "$tflite_model_int8"

    # Reclaim space
    echo "Reclaiming space..."
    deactivate
    rm -rf "$env"
    cd ..
    rm -rf "$project_folder"

    # Check if TFLM models were generated successfully
    if [[ -f "$tflm_models/$tflm_model1a" && -f "$tflm_models/$tflm_model1b" && -f "$tflm_models/$tflm_model2a" && -f "$tflm_models/$tflm_model2b" ]]; then
        echo "TFLM models have been generated successfully"
    fi
fi

# Copy models to final paths
cp "$tflm_models/$tflm_model2a" "$final_int8_model_path/$tflm_model2a"
cp "$tflm_models/$tflm_model2b" "$final_int8_model_path/$tflm_model2b"
cp "$tflm_models/$tflm_model1a" "$final_float_model_path/$tflm_model1a"
cp "$tflm_models/$tflm_model1b" "$final_float_model_path/$tflm_model1b"

# Verify copied models
if [[ -f "$final_float_model_path/$tflm_model1a" && -f "$final_float_model_path/$tflm_model1b" && -f "$final_int8_model_path/$tflm_model2a" && -f "$final_int8_model_path/$tflm_model2b" ]]; then
    echo "TFLM models have been copied into respective projects successfully, check the following paths to find the models:"
    echo "$final_int8_model_path"
    echo "$final_float_model_path"
fi