'''
@Author: Sauron Wu
@GitHub: wutianze
@Email: 1369130123qq@gmail.com
@Date: 2019-09-20 14:23:08
@LastEditors: Sauron Wu
@LastEditTime: 2019-09-25 13:53:44
@Description: 
'''
# 将图片处理为npz格式
# 自动驾驶模型真实道路模拟行驶
import os
import numpy as np
#import matplotlib.image as mpimg
from time import time
import math
from PIL import Image
import csv
import argparse
import config
import random


# this function need you to specify
def process_img(img_path, key, method):

    #print(img_path)
    # the method for processing images,0:/255,1:/255-0.5,3:/127.5-1
    image = Image.open(img_path)
    image_array = np.array(image)
    if method == 0:
        image_array = image_array/255.0
    elif method == 1:
        image_array = image_array/255.0 - 0.5
    elif method == 2:
        image_array = image_array/127.5 - 1.0
    image_array = np.expand_dims(image_array, axis=0)  # 增加一个维度

    #print(image_array.shape)
    label_array = [0.,0.,0.,0.]
    label_array[int(key[0])] = 1.

    return (image_array, label_array)
    # 返回图片的数据（矩阵），和对应的标签值


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='prediction server')
    parser.add_argument('--path', type=str, help='images dir', default="/home/sauron/pynq_car/sdsandbox/sdsim/log")
    parser.add_argument('--store', type=str, help='npz store dir', default="./training_data_npz")
    parser.add_argument('--method', type=int, help='how to process', default=0)
    args = parser.parse_args()
    path = args.path
    names = []
    keys = {}
    with open(path+"/train.csv") as f:
        files = csv.reader(f)
        for row in files:
            if row[1] == 1:
                randNum = random.randint(0,10)
                # delete some data randomly
                if randNum < 6:
                    continue
            names.append(row[0])
            tmp = []
            for i in range(1,len(row)):
                tmp.append(row[i])
            keys[row[0]] = tmp
    turns = int(math.ceil(len(names) / config.CHUNK_SIZE))      # 取整，把所有图片分为这么多轮，每CHUNK_SIZE张一轮
    print("number of files: {}".format(len(names)))
    print("turns: {}".format(turns))

    for turn in range(0, turns):
        train_labels = np.zeros((1, config.OUTPUT_NUM), 'int')           # 初始化标签数组
        train_imgs = np.zeros([1, config.IMAGE_HEIGHT, config.IMAGE_WIDTH, config.IMAGE_CHANNELS])            # 初始化图像数组

        CHUNK_files = names[turn * config.CHUNK_SIZE: (turn + 1) * config.CHUNK_SIZE] # 取出当前这一轮图片
        #print("number of CHUNK files: {}".format(len(CHUNK_files)))
        print("Turn Now:%d"%turn)
        for file in CHUNK_files:
            # 不是文件夹，并且是jpg文件
            if not os.path.isdir(file) and file[len(file) - 3:len(file)] == 'jpg':
                try:
                    key = keys[file]

                    image_array, label_array = process_img(path + "/" + file,key, args.method)
                    train_imgs = np.vstack((train_imgs, image_array))
                    train_labels = np.vstack((train_labels, label_array))
                except:
                    print('prcess error')

        # 去掉第0位的全零图像数组，全零图像数组是 train_imgs = np.zeros([1,120,160,3]) 初始化生成的
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

