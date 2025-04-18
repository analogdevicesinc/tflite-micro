/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "image_provider.h"

#include "model_settings.h"

#include <fstream>
//#include <cstdlib>

TfLiteStatus GetImage(int image_width, int image_height, int channels,
                      int8_t* image_data) {
#ifdef DUMMY_INPUT
  for (int i = 0; i < image_width * image_height * channels; ++i) {
    image_data[i] = 0;
  }
#else
  FILE *fp = fopen("../src/input/is_person.bin","rb");
  if (fp==NULL){
	  PRINT_INFO("Failed to read the input file. Check if the is_person.bin is available in input folder.Refer person_detection/Readme.");
  }
  fread(image_data, sizeof(int8_t), image_width * image_height * channels, fp);
  fclose(fp);
#endif
  return kTfLiteOk;
}
