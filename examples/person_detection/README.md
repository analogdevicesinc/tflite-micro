# Person Detection
This folder contains the SharcFX port of person_detection - An example application provided by tflite-micro. 


## Details of the model (from the original repo)
This example shows how you can use Tensorflow Lite to run a 250 kilobyte neural network to recognize people in images.

## Additional details
|Content|Supported?|
|:--------|:----------:|
|float model|:x:|
|int8 quantized model|✅|
|FileIO operation |✅|
|Realtime operation |:x:|

##  Run application in FileIO mode
* Open CCES and pull in the **person_detection** project into your CCES workspace. 
* Build and run the **person_detection** project and the output can be seen on the console. Refer to the `ADI_TFLITE_MICRO_SHARCFX_UsersGuide.doc` for more information on how to build and run a project. 
* Expected Input: Requires `is_person.bin` in `src\input`. To run it on a custom image, first convert the image to binary using the script in `Eagle-TFLM\Utils\data\scripts`. Please follow the Readme in `Eagle-TFLM\Utils\data\person_detection` to generate the input.
* Expected output: Prints the probablitiy distribution for a prediction on each class(**person** and **no-person**) on the console.


##  Run application in Realtime mode
* Not implemented