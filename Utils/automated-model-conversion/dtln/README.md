
This folder contains scripts for downloading and generating TensorFlow Lite Micro (TFLM) model files from publicly available prebuilt models [https://github.com/breizhn/DTLN.git] for the dtln project. Since these models do not have a permissive license for direct redistribution, we provide scripts to obtain and convert them instead of sharing the model files.

Ensure to have python 3.10.x(Any Stable Release like [this one](https://www.python.org/ftp/python/3.10.9/python-3.10.9-amd64.exe)) and git installed and available in your system's PATH 

Run the `enable_long_paths.reg` file to add support for long paths in Windows.

Run the `convert_models.bat` file to generate the float quantized model files for DTLN denoiser project. 

The generated model files will be populated in the `Eagle-TFLM\examples\denoiser\common\model\`
Model files in `Eagle-TFLM\examples\denoiser\common\model\model_fp` are required to build and run the application. Absence of .cc and .h files in this folder will lead to build errors.
