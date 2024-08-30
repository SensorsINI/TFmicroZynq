import os
import re
import numpy as np
import tensorflow as tf
from types import SimpleNamespace
from SI_Toolkit.Functions.General.Initialization import get_net


def tf2tf_lite(path_to_models, net_name, batch_size):
    # Parameters:
    a = SimpleNamespace()
    batch_size = batch_size
    a.path_to_models = path_to_models
    a.net_name = net_name

    # Import network
    # Create a copy of the network suitable for inference (stateful and with sequence length one)
    net, net_info = \
        get_net(a, time_series_length=1,
                batch_size=batch_size, stateful=True, remove_redundant_dimensions=True)

    # Convert the Keras model to TensorFlow Lite model
    converter = tf.lite.TFLiteConverter.from_keras_model(net)
    tflite_model = converter.convert()

    check_dtype(net, tflite_model)

    # Create folder tf_lite if it doesn't exist
    tf_lite_folder = os.path.join(path_to_models, net_name, 'tf_lite')
    if not os.path.exists(tf_lite_folder):
        os.makedirs(tf_lite_folder)

    # Save the TensorFlow Lite model
    tf_lite_model_name = net_name.replace("-", "_") + '.tflite'
    save_path = os.path.join(tf_lite_folder, tf_lite_model_name)
    with open(save_path, "wb") as f:
        f.write(tflite_model)

    print(f"TensorFlow Lite model saved to {save_path}")

    # Generate and save golden input and output
    # Generate random input
    if net_info.net_type == 'Dense':
        shape_input = (batch_size, len(net_info.inputs),)
    else:
        shape_input = (batch_size, 1, len(net_info.inputs))

    random_input = np.random.random(shape_input).astype(np.float32)

    # Run the model to get the golden output
    golden_output = net.predict(random_input)

    # Save the input as CSV and the output as .npy for testing
    golden_input_path_csv = os.path.join(tf_lite_folder, net_name.replace("-", "_") + '_golden_input.csv')
    golden_output_path_csv = os.path.join(tf_lite_folder, net_name.replace("-", "_") + '_golden_output.csv')

    # Save golden input as CSV
    np.savetxt(golden_input_path_csv, random_input.flatten(), delimiter=',', fmt='%.5f')

    # Save golden output as CSV
    np.savetxt(golden_output_path_csv, golden_output.flatten(), delimiter=',', fmt='%.5f')

    print(f"Golden input saved to {golden_input_path_csv}")
    print(f"Golden output saved to {golden_output_path_csv}")

    # Convert the .tflite model into a C++ source file using xxd
    cc_file_path = os.path.join(tf_lite_folder, net_name.replace("-", "_") + '_model.cc')
    os.system(f'xxd -i {save_path} > {cc_file_path}')

    print(f"C++ source file generated at {cc_file_path}")

    # Modify the generated .cc file to include alignas(16) and create the .h file
    h_file_path = os.path.join(tf_lite_folder, net_name.replace("-", "_") + '_model.h')
    with open(cc_file_path, 'r') as cc_file:
        cc_lines = cc_file.readlines()

    # Variables to hold the last line from the .cc file
    cc_last_line = None
    new_array_name = net_name.replace("-", "_") + "_model_array"

    # Adjust the variable declaration in the .cc file and create the .h file
    with open(cc_file_path, 'w') as cc_file, open(h_file_path, 'w') as h_file:
        # Include necessary headers in the .cc file
        cc_file.write(f'#include "{net_name.replace("-", "_")}_model.h"\n\n')

        # Add header guards to the .h file
        header_guard = f"_{net_name.replace('-', '_').upper()}_MODEL_H_"
        h_file.write(f"#ifndef {header_guard}\n")
        h_file.write(f"#define {header_guard}\n\n")
        h_file.write("#include <cstdint>\n\n")

        for line in cc_lines:
            if line.startswith("unsigned char"):
                # Modify the line to include alignas(16) and change the array name
                line = re.sub(r'unsigned char\s+\w+\[\]\s*=', f"alignas(16) const unsigned char {new_array_name}[] =", line)
            elif line.startswith("unsigned int"):
                # Capture the last line of the .cc file for later use
                cc_last_line = line
                continue  # Skip writing this line to the .cc file

            cc_file.write(line)

        if cc_last_line:
            # Modify the captured line and add to the .h file
            cc_last_line = re.sub(r'unsigned int\s+\w+\s*=', f'constexpr unsigned int {new_array_name}_size =', cc_last_line)

            h_file.write(cc_last_line)

        # End the header guard in the .h file
        h_file.write(f"extern const unsigned char {new_array_name}[];\n\n")

        h_file.write(f"#define model_array_size     {new_array_name}_size\n")
        h_file.write(f"#define model_array          {new_array_name}\n\n")

        h_file.write("#endif\n")

    print(f"Header file generated at {h_file_path}")


def check_dtype(net, tflite_model):
    # Checking format of the model
    # Load your TensorFlow Lite model from the byte buffer
    for layer in net.layers:
        if any(layer.dtype == 'float64' for layer in layer.weights):
            print(f"Layer {layer.name} contains float64 weights or biases.")

    interpreter = tf.lite.Interpreter(model_content=tflite_model)
    interpreter.allocate_tensors()

    # Get input details
    input_details = interpreter.get_input_details()
    output_details = interpreter.get_output_details()

    # Check input data type
    input_dtype = input_details[0]['dtype']
    print(f"Input data type: {input_dtype}")

    # Check output data type
    output_dtype = output_details[0]['dtype']
    print(f"Output data type: {output_dtype}")