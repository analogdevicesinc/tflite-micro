#!/bin/bash

set -e  # Exit on error
set -u  # Treat unset vars as errors

project_folder="music-genre-classification"
env="python_env"
tflite_model_float32="float32_genre_ID.tflite"
tflite_model_int8="int8_genre_ID.tflite"
tflm_models="TFLM_models"
tflm_model1a="float32_genre_ID_model_data.h"
tflm_model1b="float32_genre_ID_model_data.cc"
tflm_model2a="int8_genre_ID_model_data.h"
tflm_model2b="int8_genre_ID_model_data.cc"

final_int8_model_path="../../../examples/genre_identification/common/model/int8_genre_ID/"
final_float_model_path="../../../examples/genre_identification/common/model/float_genre_ID/"

# Function to print progress during deletion
remove_all_with_progress() {
    target="$1"
    if [ -d "$target" ]; then
        echo "Deleting $target..."
        find "$target" -type f | while read -r file; do
            echo "Deleting $file"
            rm -f "$file"
        done
        rm -rf "$target"
    fi
}

# Check if all four model files exist already
if [[ -f "$tflm_models/$tflm_model1a" && -f "$tflm_models/$tflm_model1b" && -f "$tflm_models/$tflm_model2a" && -f "$tflm_models/$tflm_model2b" ]]; then
    echo "TFLM models have been generated successfully"
else
    if [[ ! -d "$project_folder" ]]; then
        git clone https://github.com/cetinsamet/music-genre-classification.git
    else
        echo "Project directory already present, moving to next step"
    fi

    cd "$project_folder/scripts"

    if [[ ! -d "$env" ]]; then
        python3 -m pip install virtualenv
        python3 -m virtualenv "$env"
    fi

    source "$env/bin/activate"

    pip install -r requirements.txt

    if [[ ! -f "$tflite_model_int8" ]]; then
        python convert_model.py
    fi

    if [[ ! -d "$tflm_models" ]]; then
        python generate_cc_arrays.py "$tflm_models" "$tflite_model_float32"
        python generate_cc_arrays.py "$tflm_models" "$tflite_model_int8"
    fi

    # Remove old TFLM_models folder from parent (if exists) and move new one
    rm -rf ../TFLM_models
    mv TFLM_models ../

    # Delete tflite files and clean virtual env
    rm -f "$tflite_model_float32" "$tflite_model_int8"
    echo "Reclaiming space..."
    deactivate
    remove_all_with_progress "$env"

    cd ..
    rm -rf "$project_folder"

    # Final check
    if [[ -f "$tflm_models/$tflm_model1a" && -f "$tflm_models/$tflm_model1b" && -f "$tflm_models/$tflm_model2a" && -f "$tflm_models/$tflm_model2b" ]]; then
        echo "TFLM models have been generated successfully"
    fi
fi

# Copy int8 model to final path
mkdir -p "$final_int8_model_path"
cp "$tflm_models/$tflm_model2a" "$final_int8_model_path/$tflm_model2a"
cp "$tflm_models/$tflm_model2b" "$final_int8_model_path/$tflm_model2b"

# Uncomment if float32 model also needed
# mkdir -p "$final_float_model_path"
# cp "$tflm_models/$tflm_model1a" "$final_float_model_path/$tflm_model1a"
# cp "$tflm_models/$tflm_model1b" "$final_float_model_path/$tflm_model1b"

# Confirm success
if [[ -f "$final_int8_model_path/$tflm_model2a" && -f "$final_int8_model_path/$tflm_model2b" ]]; then
    echo "TFLM models copied to:"
    echo "$final_int8_model_path"
    # echo "$final_float_model_path"
fi
