'''
@Author: Sauron Wu
@GitHub: wutianze
@Email: 1369130123qq@gmail.com
@Date: 2019-09-20 14:23:08
@LastEditors: Sauron Wu
@LastEditTime: 2019-10-24 13:27:41
@Description: 
'''
import os
import numpy as np
from time import time
import math
import csv
import argparse
import random
import cv2

CHUNK_SIZE = 256
IMAGE_SHAPE = [120,160,3]
global OUTPUT_NUM
OUTPUT_NUM = 1
def image_handle(img):
    image = np.asarray(img)
    image.reshape((image.shape[0],image.shape[1],image.shape[2]))
    return (image)/255.0 - 0.5

CONV_INPUT = "conv2d_1_input"
calib_batch_size = 50
def calib_input(iter):
  images = []
  path = "/home/sauron/pynq_car/sdsandbox/sdsim/lsr-pid2/"
  files = os.listdir(path)
  for index in range(0, calib_batch_size):
    if files[iter*calib_batch_size+index] == "train.csv":
        continue
    image = image_handle(cv2.imread(path + files[iter*calib_batch_size + index]))
    images.append(image)
  return {CONV_INPUT: images}

def process_img(img_path, key):
    image_array = cv2.imread(img_path)
    label_array = []
    for k in key:
        label_array.append(float(k)) 

    image_array = image_handle(image_array)
    image_array = np.expand_dims(image_array, axis=0)
    #print(image_array.shape)  
    return (image_array, label_array)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='prediction server')
    parser.add_argument('--path', type=str, help='images dir', default="./images")
    parser.add_argument('--store', type=str, help='npz store dir', default="./process_images")
    parser.add_argument('--method', type=int, help='whether to reduce some categories\' number, 0 for true', default=0)
    args = parser.parse_args()
    path = args.path
    names = []
    keys = {}
    with open(path+"/train.csv") as f:
        files = list(csv.reader(f))
        
        image_for_shape = cv2.imread(path+'/'+files[0][0])
        IMAGE_SHAPE[0] = image_for_shape.shape[0]
        IMAGE_SHAPE[1] = image_for_shape.shape[1]
        IMAGE_SHAPE[2] = image_for_shape.shape[2]
        OUTPUT_NUM = len(files[0]) - 1
        print("OUTPUT_NUM is:%d"%OUTPUT_NUM)    
        for row in files:
            if args.method == 0:
                if row[1] == 1: # this should be set according to your training data
                    randNum = random.randint(0,10)
                    # delete some data randomly, bigger of the threshold number means more data in this category will be ignored
                    if randNum < 8:
                       continue
            names.append(row[0])
            tmp = []
            for i in range(1,len(row)):
                tmp.append(row[i])
            keys[row[0]] = tmp
    turns = int(math.ceil(len(names) / CHUNK_SIZE))      
    print("number of files: {}".format(len(names)))
    print("turns: {}".format(turns))

    for turn in range(0, turns):
        train_labels = np.zeros((1, OUTPUT_NUM), 'int')           # initialize labels
        train_imgs = np.zeros([1, IMAGE_SHAPE[0],IMAGE_SHAPE[1],IMAGE_SHAPE[2]])            # initialize image array

        CHUNK_files = names[turn * CHUNK_SIZE: (turn + 1) * CHUNK_SIZE] # get one chunk
        print("number of CHUNK files: {}".format(len(CHUNK_files)))
        print("Turn Now:%d"%turn)
        for file in CHUNK_files:
            if not os.path.isdir(file) and file[len(file) - 3:len(file)] == 'jpg':
                key = keys[file]
                image_array, label_array = process_img(path + "/" + file,key)
                train_imgs = np.vstack((train_imgs, image_array))
                train_labels = np.vstack((train_labels, label_array))

        # delete the initial all-0 array
        train_imgs = train_imgs[1:, :]
        train_labels = train_labels[1:, :]
        file_name = str(int(time()))
        directory = args.store

        if not os.path.exists(directory):
            os.makedirs(directory)
        try:
            np.savez(directory + '/' + file_name + '.npz', train_imgs=train_imgs, train_labels=train_labels)
        except IOError as e:
            print(e)

