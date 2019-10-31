'''
@Author: your name
@Date: 2019-10-29 15:33:55
@LastEditTime: 2019-10-29 15:33:55
@LastEditors: your name
@Description: In User Settings Edit
@FilePath: /pynq_car/Host-Part/dnndk-host/graph_input_fn.py
'''
import cv2
import os
import numpy as np

def image_handle(img):
    #print("img:")
    #print(img)
    #image = np.asarray(img)
    #img.reshape((img.shape[0],img.shape[1],img.shape[2]))
    return (img[40:,:])/255.0-0.5

CONV_INPUT = "conv2d_1_input"
calib_batch_size = 50
def calib_input(iter):
  images = []
  path = "/home/xilinx/real_data/"
  files = os.listdir(path)
  for index in range(0, calib_batch_size):
    if files[iter*calib_batch_size+index] == "train.csv":
        continue
    image = image_handle(cv2.imread(path + files[iter*calib_batch_size + index]))
    images.append(image)
  return {CONV_INPUT: images}