This folder contains scripts for downloading and generating TensorFlow Lite Micro (TFLM) model files from publicly available prebuilt models {https://github.com/cetinsamet/music-genre-classification.git] for the genre_identification project. Since these models do not have a permissive license for direct redistribution, we provide scripts to obtain and convert them instead of sharing the model files.

Ensure to have python 3.10.x(Any Stable Release like [this one](https://www.python.org/ftp/python/3.10.11/python-3.10.11.exe) and git installed and available in your system's PATH 

Run the `enable_long_paths.reg` file to add support for long paths in Windows.

Run the `convert_models.bat` file to generate the float and int8 quantized model files for genreID project.

The generated model files will be populated in the `Eagle-TFLM\examples\genre_identification\common\model\`
Model files in `Eagle-TFLM\examples\genre_identification\common\model\int8_genre_ID` and `Eagle-TFLM\examples\genre_identification\common\model\float_genre_ID` are required to build and run the application. Absence of .cc and .h files in this folder will lead to build errors.
