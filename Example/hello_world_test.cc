#include "tensorflow/lite/core/c/common.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_log.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_profiler.h"
#include "tensorflow/lite/micro/recording_micro_interpreter.h"
#include "tensorflow/lite/micro/system_setup.h"
#include "tensorflow/lite/schema/schema_generated.h"

// If you use the static library as described in README,
// you can replace all the above #include "tensorflow/..." with one line:
// #include "TFmicroZynqLib/tfmicrozynq.h"

#include <math.h>
#include <stdio.h>  // Include this header to use snprintf
#include <stdarg.h>  // For va_list, va_start, va_end
#include "xil_printf.h"

#include "Dense_7IN_32H1_32H2_1OUT_0_model.h"


namespace {
using HelloWorldOpResolver = tflite::MicroMutableOpResolver<2>;

TfLiteStatus RegisterOps(HelloWorldOpResolver& op_resolver) {
  TF_LITE_ENSURE_STATUS(op_resolver.AddFullyConnected());
  TF_LITE_ENSURE_STATUS(op_resolver.AddTanh()); // Register TANH operation
  return kTfLiteOk;
}
}  // namespace

TfLiteStatus ProfileMemoryAndLatency() {
  tflite::MicroProfiler profiler;
  HelloWorldOpResolver op_resolver;
  TF_LITE_ENSURE_STATUS(RegisterOps(op_resolver));

  // Arena size just a round number. The exact arena usage can be determined
  // using the RecordingMicroInterpreter.
  constexpr int kTensorArenaSize = 12000;
  uint8_t tensor_arena[kTensorArenaSize];
  constexpr int kNumResourceVariables = 24;

  tflite::RecordingMicroAllocator* allocator(
      tflite::RecordingMicroAllocator::Create(tensor_arena, kTensorArenaSize));
  tflite::RecordingMicroInterpreter interpreter(
      tflite::GetModel(model_array), op_resolver, allocator,
      tflite::MicroResourceVariables::Create(allocator, kNumResourceVariables),
      &profiler);

  TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());
  unsigned int num_of_inputs = 7;

  TFLITE_CHECK_EQ(interpreter.inputs_size(), 1); // This is number of input tensors, not the input tensor size. This should be for us always 1
  for (unsigned int k = 0; k < num_of_inputs; k++)
  {
  	interpreter.input(0)->data.f[k] = 1.f;
  }
//  interpreter.input(0)->data.f[0] = 1.f;
  TF_LITE_ENSURE_STATUS(interpreter.Invoke());

  MicroPrintf("");  // Print an empty new line
  profiler.LogTicksPerTagCsv();

  MicroPrintf("");  // Print an empty new line
  interpreter.GetMicroAllocator().PrintAllocations();
  return kTfLiteOk;
}

TfLiteStatus LoadFloatModelAndPerformInference() {
  const tflite::Model* model =
      ::tflite::GetModel(model_array);
  TFLITE_CHECK_EQ(model->version(), TFLITE_SCHEMA_VERSION);

  HelloWorldOpResolver op_resolver;
  TF_LITE_ENSURE_STATUS(RegisterOps(op_resolver));

  // Arena size just a round number. The exact arena usage can be determined
  // using the RecordingMicroInterpreter.
  constexpr int kTensorArenaSize = 12000;
  uint8_t tensor_arena[kTensorArenaSize];

  tflite::MicroInterpreter interpreter(model, op_resolver, tensor_arena,
                                       kTensorArenaSize);
  TF_LITE_ENSURE_STATUS(interpreter.AllocateTensors());

  // Check if the predicted output is within a small range of the
  // expected output
  float epsilon = 0.05f;
  constexpr int kNumTestValues = 1;

//  float golden_inputs[kNumTestValues] = {0.f, 1.f, 3.f, 5.f};
//  float golden_outputs[kNumTestValues];
//  for (int i = 0; i < kNumTestValues; ++i) {
//	  double golden_input = golden_inputs[i];
//	  golden_outputs[i] = sin(golden_input);
//  }

//  float golden_inputs[kNumTestValues] = {0.37579, 0.37579, 0.37579, 0.37579};
//  float golden_outputs[kNumTestValues] = {-0.11439, -0.11439, -0.11439, -0.11439};

  int num_of_inputs = 7;
  float golden_inputs[kNumTestValues][num_of_inputs] = {{0.57892, 0.01099, 0.28099, 0.13712, 0.81903, 0.88581, 0.48020}};
  float golden_outputs[kNumTestValues][1] = {{-0.53476}};

//  int num_of_inputs = 1;
//  float golden_inputs[kNumTestValues][num_of_inputs] = {{0.37579}};
//  float golden_outputs[kNumTestValues][1] = {{-0.11439}};

  for (int i = 0; i < kNumTestValues; ++i) {
	float golden_input = golden_inputs[i][0];
	float expected_output = golden_outputs[i][0];
    // Fill input buffer (use test value)
    for (int k = 0; k < num_of_inputs; k++)
    {
    	interpreter.input(0)->data.f[k] = golden_inputs[i][k];
    }
//    interpreter.input(0)->data.f[0] = golden_input;
    TF_LITE_ENSURE_STATUS(interpreter.Invoke());
    float y_pred = interpreter.output(0)->data.f[0];
    TFLITE_CHECK_LE(abs(expected_output - y_pred), epsilon);
  }

  return kTfLiteOk;
}

int minimal_time()
{

}

int main(int argc, char* argv[]) {
  tflite::InitializeTarget();
  TF_LITE_ENSURE_STATUS(ProfileMemoryAndLatency());
  TF_LITE_ENSURE_STATUS(LoadFloatModelAndPerformInference());

  print("~~~ALL TESTS PASSED~~~\n");
  return kTfLiteOk;
}
