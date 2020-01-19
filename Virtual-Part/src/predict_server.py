'''
@Author: Sauron Wu
@GitHub: wutianze
@Email: 1369130123qq@gmail.com
@Date: 2019-09-23 10:12:28
@LastEditors  : Please set LastEditors
@LastEditTime : 2020-01-19 12:58:44
@Description: 
'''
#!/usr/bin/env python
'''
Predict Server
Create a server to accept image inputs and run them against a trained neural network.
This then sends the steering output back to the client.
Author: Tawn Kramer
'''
#from __future__ import print_function
import os
import argparse
import json
import time
import asyncore
import socket
from io import BytesIO
import base64
import datetime
import tensorflow as tf

import numpy as np
from keras.models import load_model
from PIL import Image
import cv2
from tcp_server import IMesgHandler, SimServer

class PynqSimMsgHandler(IMesgHandler):

    def __init__(self, model, port=0, control_method=0):
        self.model = model
        self.graph = tf.get_default_graph()
        self.sock = None
        self.image_folder = None
        self.steering_angle = 0.
        self.throttle = 0.
        self.num_cars = 0
        self.port = port
        self.target_num_cars = 1
        self.control_method = control_method
        self.fns = {'telemetry' : self.on_telemetry,\
                    'car_loaded' : self.on_car_created,\
                    'on_disconnect' : self.on_disconnect}

    def on_connect(self, socketHandler):
        self.sock = socketHandler

    def on_disconnect(self):
        self.num_cars = 0

    def on_recv_message(self, message):
        if not 'msg_type' in message:
            print('expected msg_type field')
            return

        msg_type = message['msg_type']
        if msg_type in self.fns:
            self.fns[msg_type](message)
        else:
            print('unknown message type', msg_type)

    def on_car_created(self, data):

        self.num_cars += 1
        if self.num_cars < self.target_num_cars:
            print("requesting another car..")
            self.request_another_car()

    def on_telemetry(self, data):
        imgString = data["image"]
        #image = Image.open(BytesIO(base64.b64decode(imgString)))
        #image_array = cv2.cvtColor(np.array(image),cv2.COLOR_RGB2BGR)
        image_array = cv2.imdecode(np.fromstring(base64.b64decode(imgString),dtype=np.uint8),1)

        image_array = image_array[40:,:]
        image_array = image_array/255.0-0.5
        #print(image_array.shape)
        self.predict(image_array)

        # maybe save frame
        if self.image_folder is not None:
            timestamp = datetime.utcnow().strftime('%Y_%m_%d_%H_%M_%S_%f')[:-3]
            image_filename = os.path.join(self.image_folder, timestamp)
            image.save('{}.jpg'.format(image_filename))


    def predict(self, image_array):
        with self.graph.as_default():
            outputs = self.model.predict(image_array[None, :, :, :])
        #outputs = np.array([0,1,0,0])
        #rint("predict outputs")
        print(outputs)
        self.parse_outputs(outputs)
    
    def parse_outputs(self, outputs):
        res = []
        for output in outputs:            
            for i in range(output.shape[0]):
                res.append(output[i])

        self.on_parsed_outputs(res)

    def on_parsed_outputs(self, outputs):
        # softmax output
        if self.control_method == 0:
            comSend = ''
            toSend = 0
            nowMax = 0.0
            for i in range(len(outputs)):
                if outputs[i] > nowMax:
                    nowMax = outputs[i] 
                    toSend = i
            if toSend == 0:
                comSend = 'a'
            elif toSend == 1:
                comSend = 'w'
            elif toSend == 2:
                comSend = 'd'
            else:
                comSend = 's'
            self.send_control0(comSend)
        else:# regression model
            self.send_control1(outputs[0],outputs[1])# send steering and speed

        #if self.control_method == 0:
        #    comSend = ''
        #    toSend = 0
        #    nowMax = 0.0
        #    for i in range(len(outputs)):
        #        if outputs[i] > nowMax:
        #            nowMax = outputs[i] 
        #            toSend = i
        #    if toSend == 0:
        #        comSend = 'a'
        #    elif toSend == 1:
        #        comSend = 'w'
        #    elif toSend == 2:
        #        comSend = 'd'
        #    else:
        #        comSend = 's'
        #    self.send_control0(comSend)
        #elif self.control_method == 1:
        #    #*4 to make the car more sensitive
        #    steer_com = ((outputs[0]*2.)-1.)*4.
        #    print(steer_com)
        #    self.send_control1(steer_com, 0.3)
        #elif self.control_method == 2:
        #    #print("on parsed outputs")
        #    print(outputs)
        #    comSend = ''
        #    toSend = 0
        #    nowMax = 0.0
        #    outputs[1] = outputs[1] -0.2
        #    for i in range(len(outputs)):
        #        if outputs[i] > nowMax:
        #            nowMax = outputs[i] 
        #            toSend = i
        #    steerSend = 0.0
        #    if toSend == 0:
        #        steerSend = -0.5
        #    elif toSend == 2:
        #        steerSend = 0.5
        #    self.send_control1(steerSend, 0.3)

        
    def send_control0(self, command):
        msg = { 'msg_type' : 'pynq_command', 'command':command[0] }
        self.sock.queue_message(msg)

    def send_control1(self, steer, speed):
        steer = steer*2.0 - 1.0
        speed = speed*2.0 - 1.0
        msg = { 'msg_type' : 'pynq_speed', 'steering': steer.__str__(), 'speed':speed.__str__()}
        self.sock.queue_message(msg) 

    def on_close(self):
        pass

def go(filename, address, control_method=0):

    model = load_model(filename)
    handler = PynqSimMsgHandler(model,control_method=control_method)
    server = SimServer(address, handler)

    try:
        #asyncore.loop() will keep looping as long as any asyncore dispatchers are alive
        asyncore.loop()
    except KeyboardInterrupt:
        #unless some hits Ctrl+C and then we get this interrupt
        print('stopping')

# ***** main loop *****
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='prediction server')
    parser.add_argument('--model', type=str, help='model filename')
    parser.add_argument('--host', type=str, default='0.0.0.0', help='bind to ip')
    parser.add_argument('--port', type=int, default=9090, help='bind to port')
    #parser.add_argument('--constant_throttle', type=float, default=0.0, help='apply constant throttle')
    parser.add_argument('--control_method', type=int, default=1, help='0 for command, 1 for steer')
    args = parser.parse_args()

    address = (args.host, args.port)
    #print(args.control_method)
    go(args.model, address,control_method=args.control_method)
