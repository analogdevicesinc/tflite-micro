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


// This is the default main used on systems that have the standard C entry
// point. Other devices (for example FreeRTOS or ESP32) that have different
// requirements for entry code (like an app_main function) should specialize
// this main.cc file in a target-specific subfolder.
#include "unit_test.h"

namespace micro_test {
  int tests_passed;
  int tests_failed;
  bool is_test_complete;
  bool did_test_fail;
  };

int main()
{
	MicroPrintf("\n>>>>>>>>>>fully connected test start<<<<<<<<<<\n");
	fullyConnected();
	MicroPrintf("\n\n----------fully connected test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>DepthWise convolution test start<<<<<<<<<<\n");
	DepthWise_conv();
	MicroPrintf("\n\n----------DepthWise convolution test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>lstm eval test start<<<<<<<<<<\n");
	lstmEval();
	MicroPrintf("\n\n----------lstm eval test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>convolution test start<<<<<<<<<<\n");
	convolutionTest();
	MicroPrintf("\n\n----------convolution test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>softmax test start<<<<<<<<<<\n");
	softmaxTest();
	MicroPrintf("\n\n----------softmax test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>tanh test start<<<<<<<<<<\n");
	tanhTest();
	MicroPrintf("\n\n----------tanh test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>activations test start<<<<<<<<<<\n");
	activationsTest();
	MicroPrintf("\n\n----------activations test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>concatenation test start<<<<<<<<<<\n");
	concatenationTest();
	MicroPrintf("\n\n----------concatenation test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>logistic test start<<<<<<<<<<\n");
	logisticTest();
	MicroPrintf("\n\n----------logistic test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>addTest start<<<<<<<<<<\n");
	addTest();
	MicroPrintf("\n\n----------addTest completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>addNtest start<<<<<<<<<<\n");
	addNtest();
	MicroPrintf("\n\n----------addNtest completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>agrMinMax test start<<<<<<<<<<\n");
	agrMinMaxTest();
	MicroPrintf("\n\n----------agrMinMax test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>leakyRelu test start<<<<<<<<<<\n");
	leakyReluTest();
	MicroPrintf("\n\n----------leakyRelu test completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>preluTest start<<<<<<<<<<\n");
	preluTest();
	MicroPrintf("\n\n----------preluTest completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>quantizeTest start<<<<<<<<<<\n");
	quantizeTest();
	MicroPrintf("\n\n----------quantizeTest completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>poolingTest start<<<<<<<<<<\n");
	poolingTest();
	MicroPrintf("\n\n----------poolingTest completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>transposeConvTest start<<<<<<<<<<\n");
	transposeConvTest();
	MicroPrintf("\n\n----------transposeConvTest completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>unidirectionalSequenceLstmTest start<<<<<<<<<<\n");
	unidirectionalSequenceLstmTest();
	MicroPrintf("\n\n----------unidirectionalSequenceLstmTest completed----------\n\n");

	MicroPrintf("\n>>>>>>>>>>eluTest start<<<<<<<<<<\n");
	eluTest();
	MicroPrintf("\n\n----------eluTest completed----------\n\n");

	return 0;
}
