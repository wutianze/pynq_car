import subprocess
import sys
import termios
import tty
import time
import os
import threading
import numpy
from PIL import Image
from PyV4L2Camera.camera import Camera
import control
import csv

lock = threading.Lock()

height = 120
width = 160
camera = Camera('/dev/video0',width,height)
# status: 0:forward, 1:left, 2:right, 3:stop
status = 3
def readchar():
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        ch = sys.stdin.read(1)
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
    return ch

def readkey(getchar_fn=None):
    getchar = getchar_fn or readchar
    c1 = getchar()
    if ord(c1) != 0x1b:
        return c1
    c2 = getchar()
    if ord(c2) != 0x5b:
        return c1
    c3 = getchar()
    return chr(0x10 + ord(c3) - 65)

class capTask:
    def __init__(self):
        self._running = True

    def terminate(self):
        self._running = False

    def run(self):
        print("***Camera Start***")
        while(self._running):
            time.sleep(2)
            lock.acquire()
            camera.get_frame()
            camera.get_frame()
            camera.get_frame()
            camera.get_frame()
            camera.get_frame()
            camera.get_frame()
            frame = camera.get_frame()
            if not frame:
                continue
            global status
            s = status
            lock.release()
            #subprocess.getstatusoutput("fswebcam -d /dev/video0 -r %sx%s ./images/%s%s%s.jpg"%(height,width,s1,s2,time.time()))
            
            im = Image.frombytes('RGB',(width,height),frame,'raw','RGB')
            im.save('./images/%s%s.jpg'%(s,time.time()))
            #arr = numpy.asarray(im)

# the value is from 0-8
def forward():
    global status
    lock.acquire()
    status = 0
    lock.release()

def stop():
    global status
    lock.acquire()
    status = 3
    lock.release()

def left():
    global status
    lock.acquire()
    status = 1
    lock.release()

def right():
    global status
    lock.acquire()
    status = 2
    lock.release()

print("***Collect data Start***")
if not os.path.exists('images'):
    os.mkdir('images')
con = control.Control()
cap = capTask()
t = threading.Thread(target=cap.run)
t.start()
print("***Please Start Now***")
while True:
    key=readkey()
    if key=='w':
        forward()
    elif key=='s':
        stop()
    elif key=='a':
        left()
    elif key=='d':
    	right()
    elif key=='q':
        cap.terminate()
        con.stop()
    global status
    con.statusAction(status)
    #time.sleep(1)
    print("\rkey:%s, status:%d"%(key,status),end = ' ')
t.join()
camera.close()
print("\n***Stop Collect***")
