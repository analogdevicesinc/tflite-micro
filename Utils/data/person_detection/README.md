python library prerequisites: opencv, pillow
can be installed with: `pip install opencv-python pillow`

1. **Sample Data**
The sample data no_person.bmp, person.bmp were downloaded from https://github.com/tensorflow/tflite-micro/tree/main/tensorflow/lite/micro/examples/person_detection/testdata and place in sample_data folder.

2. **Input/Output data for File I/O application**
The sample image file needs to be converted into binary file and to be placed in `Eagle-TFLM\examples\person_detection\src\input` as `is_person.bin`.

Inorder to convert the input image file to .bin file for use in person_detection project use the `Utils/data/scripts/get_binary_img.py`

cd Utils/data/scripts
Usage: `python get_binary_img.py [source image] [output bin]`
Example: `python get_binary_img.py "..\person_detection\sample_data\person.bmp" "..\..\..\examples\person_detection\src\input\is_person.bin"`
