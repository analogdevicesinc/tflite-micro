The Utils folder contains utility scripts that might be needed for each project for model file, sample input and output data generation and validation. 
This folder is not a standalone part of the entire project or any application, but rather a supplimentary part.

--> automated-model-conversion/
	--> dtln/
	--> genre_identification/
	This folder contains scripts for downloading and generating TensorFlow Lite Micro (TFLM) model files from publicly available prebuilt models for the dtln and genre_identification projects. Since these models do not have a permissive license for direct redistribution, we provide scripts to obtain and convert them instead of sharing the model files.

--> data/

	Contains subfolders for different example applications, each with instructions to obtain sample input data and convert it into the required format.

    --> denoiser/
	--> genre_identification/
	--> keyword_spotter/
	--> micro_speech/
	--> person_detection/

	--> scripts/
	Contains scripts for converting input and output data into the required format, following the instructions provided in the README files of each example application folder.
	
--> flashing-tools
	Contains CCES porject and instructions for flashing the pre-built realtime example applications to the sharc-fx board.