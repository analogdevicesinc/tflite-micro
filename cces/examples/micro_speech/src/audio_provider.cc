/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#include "audio_provider.h"

#include "micro_features/micro_model_settings.h"

#include "testdata/micro_speech_audio_data.h"



int32_t yes_index_count=0,no_index_count=0; //input data index
char audio_buffer_name[100];
int32_t count =0;
namespace {
int16_t g_audio_data[kMaxAudioSampleSize];
int32_t g_latest_audio_timestamp = 0;
}  // namespace

TfLiteStatus GetAudioSamples(int start_ms, int duration_ms,
                             int* audio_samples_size, int16_t** audio_samples) {

	 *audio_samples_size = kMaxAudioSampleSize;
	  for (int i = 0,j = count; (i < kMaxAudioSampleSize) && (j < count+kMaxAudioSampleSize); i++, j++)
	  {

		  g_audio_data[i] = g_micro_speech_audio_data[j];
	  }
	  *audio_samples = g_audio_data;
	  if((count+kMaxAudioSampleSize)>g_micro_speech_audio_data_size) count = 0;//if index of input reaches its size(16000) again restart from zero.

	  count += kMaxAudioSampleSize; //incrementing index of input with kMaxAudioSampleSize(512).

  return kTfLiteOk;
}

int32_t LatestAudioTimestamp() {
  g_latest_audio_timestamp += 100;

  return g_latest_audio_timestamp;
}

