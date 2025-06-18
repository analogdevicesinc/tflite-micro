# This script clones the tflite-micro repository and creates a TFLM base tree.
# It then copies the necessary source files to the TFLM directory in the current project.
# It is intended to be run from the root of the project.
#!/usr/bin/env bash
set -e -x

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
TFLITE_LIB_DIR="${ROOT_DIR}/"

cd "${TFLITE_LIB_DIR}"
TEMP_DIR=$(mktemp -d)
cd "${TEMP_DIR}"

echo Cloning tflite-micro repo to "${TEMP_DIR}"
git clone --depth 1 --single-branch "https://github.com/tensorflow/tflite-micro.git"
cd tflite-micro

# Create the TFLM base tree
echo "Creating TFLM base tree in ${TEMP_DIR}/tflm-out"
python3 tensorflow/lite/micro/tools/project_generation/create_tflm_tree.py \
  -e hello_world -e micro_speech -e person_detection "${TEMP_DIR}/tflm-out"

# Copy the TFLM source files to the new tree
echo "Copying TFLM source files to ${TFLITE_LIB_DIR}"
cd "${TFLITE_LIB_DIR}"
rm -rf TFLM/src/tensorflow
rm -rf TFLM/src/third_party
mv "${TEMP_DIR}/tflm-out/tensorflow" TFLM/src/tensorflow
mkdir -p TFLM/src/third_party/
/bin/cp -r "${TEMP_DIR}"/tflm-out/third_party/* TFLM/src/third_party/

rm -rf "${TEMP_DIR}"
