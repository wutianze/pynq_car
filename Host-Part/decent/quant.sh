#!/bin/bash


# activate DECENT_Q Python3.6 virtual environment
#conda activate decent_q3

# generate calibraion images and list file
#python generate_images.py

# remove existing files
rm -rf ./quantize_results


# run quantization
echo "#####################################"
echo "QUANTIZE"
echo "#####################################"
decent_q quantize \
  --input_frozen_graph ../model.pb \
  --input_nodes conv2d_1_input \
  --input_shapes ?,120,160,3 \
  --output_nodes dense_2/Softmax \
  --method 1 \
  --input_fn graph_input_fn.calib_input \
  --gpu 0 \
  --calib_iter 1

echo "#####################################"
echo "QUANTIZATION COMPLETED"
echo "#####################################"

