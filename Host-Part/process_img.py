'''
@Author: Sauron Wu
@GitHub: wutianze
@Email: 1369130123qq@gmail.com
@Date: 2019-09-20 14:23:08
@LastEditors: Sauron Wu
@LastEditTime: 2019-10-12 17:58:21
@Description: 
'''
import os
import numpy as np
from time import time
import math
from PIL import Image
import csv
import argparse
import config
import random


# this function need you to specify
def process_img(img_path, key):
    #print(img_path)
    image = Image.open(img_path)
    image_array = np.array(image)
    image_array = np.expand_dims(image_array, axis=0)  
    #print(image_array.shape)
    label_array = []
    for k in key:
        label_array.append(float(k)) 
    return (image_array, label_array)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='prediction server')
    parser.add_argument('--path', type=str, help='images dir', default="/home/sauron/pynq_car/sdsandbox/sdsim/lsr-pid2")
    parser.add_argument('--store', type=str, help='npz store dir', default="./training_lsr_npz3")
    parser.add_argument('--method', type=int, help='whether to reduce some categories\' number, 0 for true', default=0)
    args = parser.parse_args()
    path = args.path
    names = []
    keys = {}
    with open(path+"/train.csv") as f:
        files = csv.reader(f)
        for row in files:
            if args.method == 0:
                if row[1] == 1:
                    randNum = random.randint(0,10)
                    # delete some data randomly
                    if randNum < 8:
                       continue
            names.append(row[0])
            tmp = []
            for i in range(1,len(row)):
                tmp.append(row[i])
            keys[row[0]] = tmp
    turns = int(math.ceil(len(names) / config.CHUNK_SIZE))      
    print("number of files: {}".format(len(names)))
    print("turns: {}".format(turns))

    for turn in range(0, turns):
        train_labels = np.zeros((1, config.OUTPUT_NUM), 'int')           # initialize labels
        train_imgs = np.zeros([1, config.IMAGE_HEIGHT, config.IMAGE_WIDTH, config.IMAGE_CHANNELS])            # initialize image array

        CHUNK_files = names[turn * config.CHUNK_SIZE: (turn + 1) * config.CHUNK_SIZE] # get one chunk
        #print("number of CHUNK files: {}".format(len(CHUNK_files)))
        print("Turn Now:%d"%turn)
        for file in CHUNK_files:
            if not os.path.isdir(file) and file[len(file) - 3:len(file)] == 'jpg':
                try:
                    key = keys[file]

                    image_array, label_array = process_img(path + "/" + file,key)
                    train_imgs = np.vstack((train_imgs, image_array))
                    train_labels = np.vstack((train_labels, label_array))
                except:
                    print('prcess error')

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

