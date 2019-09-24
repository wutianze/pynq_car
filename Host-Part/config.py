'''
@Author: Sauron Wu
@GitHub: wutianze
@Email: 1369130123qq@gmail.com
@Date: 2019-09-20 14:23:08
@LastEditors: Sauron Wu
@LastEditTime: 2019-09-24 17:53:29
@Description: 
'''
# 全局变量
IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNELS = 120, 160, 3
INPUT_SHAPE = (IMAGE_HEIGHT, IMAGE_WIDTH, IMAGE_CHANNELS)
OUTPUT_NUM = 4

CHUNK_SIZE = 256    # 将图片压缩，每256个做一次处理
PROCESS_METHOD = 0
