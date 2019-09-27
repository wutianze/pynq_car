'''
@Author: Sauron Wu
@GitHub: wutianze
@Email: 1369130123qq@gmail.com
@Date: 2019-09-20 14:23:08
@LastEditors: Sauron Wu
@LastEditTime: 2019-09-27 17:05:09
@Description: 
'''
import keras
import tensorflow
import sys
import os
import h5py
import numpy as np
import glob
import random
from sklearn.model_selection import train_test_split
from keras.models import Sequential
from keras.layers import Lambda, Conv2D, MaxPooling2D, Dropout, Dense, Flatten, Cropping2D,BatchNormalization
from keras.models import load_model, Model, Input
from keras.callbacks import ModelCheckpoint, EarlyStopping, TensorBoard, ReduceLROnPlateau
from keras.optimizers import Adam, SGD
import config
import argparse
np.random.seed(0)

# step1,载入数据，并且分割为训练和验证集
# 问题，数据集太大了，已经超过计算机内存
def load_data(read_path):
    training_data = glob.glob(read_path + '/*.npz')
    # 匹配所有的符合条件的文件，并将其以list的形式返回。
    print("匹配完成。开始读入")
    print("一共%d轮"%len(training_data))

    # if no data, exit，容错判断
    if not training_data:
        print("No training data in directory, exit")
        sys.exit()
    cut = int(len(training_data)*0.8)
    return training_data[0:cut], training_data[cut:]



# step2 建立模型
def build_model(keep_prob,model_path):
    if os.path.exists(model_path+"/model.h5"):
        model = load_model(model_path+"/model.h5")
        return model
    print("开始编译模型")
#    model = Sequential()
#    model.add(Conv2D(24, (5, 5), activation='relu', strides=(2, 2), input_shape = config.INPUT_SHAPE))
#    model.add(Dropout(keep_prob))
#    model.add(Conv2D(36, (5, 5), activation='relu', strides=(2, 2)))
#    model.add(Dropout(keep_prob))
#    model.add(Conv2D(48, (5, 5), activation='relu', strides=(2, 2)))
#    model.add(Dropout(keep_prob))
#    model.add(Conv2D(64, (3, 3),activation='relu'))
#    model.add(Dropout(keep_prob))
#    model.add(Conv2D(64, (3, 3),activation='relu'))
#    model.add(Dropout(keep_prob))  # Dropout将在训练过程中每次更新参数时随机断开一定百分比（p）的输入神经元连接
#    model.add(Flatten())
#    model.add(Dense(500, activation='relu'))
#    model.add(Dense(250, activation='relu'))
#    model.add(Dense(50, activation='relu'))
#    model.add(Dense(config.OUTPUT_NUM, activation='softmax'))
#    model.summary()
    model = Sequential()

    #model.add(Cropping2D(cropping=((40,0), (0,0)), input_shape=config.INPUT_SHAPE))
    #model.add(Lambda(lambda x: x/127.5 - 1.))
    #model.add(BatchNormalization())
    model.add(Conv2D(24, (5, 5), strides=(2, 2), activation="relu",input_shape=(config.IMAGE_HEIGHT-40, config.IMAGE_WIDTH, config.IMAGE_CHANNELS)))
    model.add(Dropout(keep_prob))
    model.add(Conv2D(32, (5, 5), strides=(2, 2), activation="relu"))
    model.add(Dropout(keep_prob))
    model.add(Conv2D(48, (5, 5), strides=(2, 2), activation="relu"))
    model.add(Dropout(keep_prob))
    model.add(Conv2D(64, (3, 3), strides=(2, 2), activation="relu"))
    model.add(Dropout(keep_prob))
    model.add(Conv2D(64, (3, 3), strides=(1, 1), activation="relu"))    
    model.add(Dropout(keep_prob))
    model.add(Flatten())
    model.add(Dense(100, activation="relu"))
    model.add(Dropout(keep_prob))
    model.add(Dense(50, activation="relu"))
    model.add(Dropout(keep_prob))
    model.add(Dense(config.OUTPUT_NUM,activation='linear'))

    return model

# step3 训练模型
def train_model(model, learning_rate, nb_epoch, samples_per_epoch,
                batch_size, train_list, valid_list, model_path,method):
    if not os.path.exists(model_path+'/'):
        os.mkdir(model_path+'/')
    # 值保存最好的模型存下来
    checkpoint = ModelCheckpoint(model_path+'/model-{epoch:03d}.h5',
                                 monitor='val_loss',
                                 verbose=0,
                                 save_best_only=False,
                                 mode='min')
    # EarlyStopping patience：当earlystop被激活（如发现loss相比上一个epoch训练没有下降），
    # 则经过patience个epoch后停止训练。
    # mode：‘auto’，‘min’，‘max’之一，在min模式下，如果检测值停止下降则中止训练。在max模式下，当检测值不再上升则停止训练。
    early_stop = EarlyStopping(monitor='val_loss', min_delta=.0005, patience=20,
                               verbose=1, mode='min')
    tensorboard = TensorBoard(log_dir='./logs', histogram_freq=0, batch_size=20, write_graph=True,write_grads=True,
                              write_images=True, embeddings_freq=0, embeddings_layer_names=None,
                              embeddings_metadata=None)
    # reduce learning rate
    reduce_lr = ReduceLROnPlateau(monitor='val_loss', factor=0.1, patience=10, 
                                  verbose=0, mode='min', min_delta=1e-5,cooldown=3, min_lr=0)

    # 编译神经网络模型，loss损失函数，optimizer优化器， metrics列表，包含评估模型在训练和测试时网络性能的指标
    #model.compile(loss='mean_squared_error', optimizer=keras.optimizers.Adam(lr=learning_rate), metrics=['accuracy'])
    model.compile(optimizer="adam",loss="mse")
    # 训练神经网络模型，batch_size梯度下降时每个batch包含的样本数，epochs训练多少轮结束，
    # verbose是否显示日志信息，validation_data用来验证的数据集
    model.fit_generator(batch_generator(train_list, batch_size,method),
                        steps_per_epoch=samples_per_epoch/batch_size,
                        epochs = nb_epoch,
                        max_queue_size=1,
                        validation_data=batch_generator(valid_list, batch_size,method),
                        validation_steps=(len(valid_list)*config.CHUNK_SIZE)/batch_size,
                        callbacks=[tensorboard, checkpoint, early_stop, reduce_lr],
                        verbose=2)

# step4
# 可以一个batch一个batch进行训练，CPU和GPU同时开工
def batch_generator(name_list, batch_size,method):
    i = 0
    while True:
        # load
        if method == 4:
            image_array = np.zeros((1, config.IMAGE_HEIGHT-40, config.IMAGE_WIDTH, config.IMAGE_CHANNELS))               # 初始化
        else:
            image_array = np.zeros((1, config.IMAGE_HEIGHT, config.IMAGE_WIDTH, config.IMAGE_CHANNELS))
        label_array = np.zeros((1, config.OUTPUT_NUM), 'float')
        # every time read <=10 pack and shuffle
        for read_num in range(10):
            single_npz = name_list[random.randint(0,len(name_list)-1)]
        
            with np.load(single_npz) as data:
            #print(data.keys())
                train_temp = data['train_imgs']
                print(train_temp.shape)
                if method == 4:
                    train_temp = train_temp[:,40:,:]
                print(train_temp.shape)
                train_labels_temp = data['train_labels']
                #print(train_labels_temp)
                image_array = np.vstack((image_array, train_temp)) # 把文件读取都放入，内存
                label_array = np.vstack((label_array, train_labels_temp))
        X = image_array[1:, :]
        y = label_array[1:, :]
        if method == 0:
            X = X/255.0
        elif method == 1:
            X = X/255.0 - 0.5
        elif method == 2:
            X = X/127.5 - 1.0
        elif method == 3:
            X = X/102.83 - 1.0
        #print(X[0])
        #X = (X/255.0) this is done in process_img
        #print('Image array shape: ' + str(X.shape))
        #print('Label array shape: ' + str(y.shape))
        #print(np.mean(X))
        #print(np.var(X))

        if method == 4:
           images = np.empty([batch_size, config.IMAGE_HEIGHT-40, config.IMAGE_WIDTH, config.IMAGE_CHANNELS])
        else:
           images = np.empty([batch_size, config.IMAGE_HEIGHT, config.IMAGE_WIDTH, config.IMAGE_CHANNELS])
        steers = np.empty([batch_size, config.OUTPUT_NUM])
        #for index in np.random.permutation(X.shape[0]):
        for index in range(X.shape[0]):
            images[i] = X[index]
            steers[i] = y[index]
            #print(y[index])
            i += 1
            if i == batch_size:
                i = 0
                yield (images, steers)

    


# step5 评估模型
#def evaluate(x_test, y_test):
    #score = model.evaluate(x_test, y_test, verbose=0)
    #print('Test loss:', score[0])
    #print('Test accuracy:', score[1])


def main(model_path, read_path,method):

    # 打印出超参数

    print('-'*30)
    print('parameters')
    print('-'*30)


    keep_prob = 0.2
    learning_rate = 0.01
    nb_epoch = 50
    samples_per_epoch = 3200
    batch_size = 64

    print('keep_prob = ', keep_prob)
    print('learning_rate = ', learning_rate)
    print('nb_epoch = ', nb_epoch)
    print('samples_per_epoch = ', samples_per_epoch)
    print('batch_size = ', batch_size)
    print('-' * 30)

    # 开始载入数据
    train_list, valid_list = load_data(read_path)
    print("数据加载完毕")
    # 编译模型
    model = build_model(keep_prob,model_path)
    # 在数据集上训练模型，保存成model.h5
    train_model(model, learning_rate, nb_epoch, samples_per_epoch, batch_size, train_list, valid_list,model_path,method)
    print("模型训练完毕")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='prediction server')
    parser.add_argument('--model', type=str, help='model dir', default="./model")
    parser.add_argument('--read', type=str, help='npz store dir', default="./training_data_npz")
    parser.add_argument('--method', type=int, help='how to process', default=0)
    args = parser.parse_args()
    print(config.INPUT_SHAPE)
    main(args.model,args.read,args.method)
