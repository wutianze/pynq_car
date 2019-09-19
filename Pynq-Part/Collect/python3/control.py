from pynq import Xlnk
from pynq import Overlay
import time
#import numpy as np

class Control:
    def __init__(self,bit_path="./PYNQ-Car.bit"):
        self.pynq_car = Overlay(bit_path)
        self.leds = self.pynq_car.axi_gpio_0
        self.buttons = self.pynq_car.axi_gpio_1
        self.leds.write(0x04,0x00)
        self.leds.write(0x00,0x0F)
        self.timer0 = self.pynq_car.axi_timer_0
        self.timer1 = self.pynq_car.axi_timer_1
        
        # define the value of different levels
        self.level2speed = {0:90000,1:100000,2:110000,3:120000,4:140000,5:155000,6:165000,7:175000,8:185000}
        self.level2steer = {0:70000,1:77500,2:85000,3:92500,4:100000,5:107500,6:115000,7:122500,8:130000}
        
        # set the pulse_width and it shouldn't be changed later
        self.timer0.write(0x04,500000)
        self.timer1.write(0x04,2000000)
        
        # init and start
        self._pwm0(self.level2speed[4])
        self._pwm0_start()
        self._pwm1(self.level2steer[4])
        self._pwm1_start()
        # you can refer to nowStatus for real time info
        self.nowStatus = [4,4]
        print("***Control init finish***")

    def _pwm0(self, high_width):
        self.timer0.write(0x14,high_width) #Pulse High Width

    def _pwm0_start(self):
        self.timer0.write(0x10,0b001010010110) #enable PWM
        self.timer0.write(0x00,0b011010010110)

    def _pwm0_stop(self):
        self.timer0.write(0x00,0b000000010110)

    def _pwm1(self, high_width):
        self.timer1.write(0x14,high_width) #Pulse High Width


    def _pwm1_start(self):
        self.timer1.write(0x10,0b001010010110) #enable PWM
        self.timer1.write(0x00,0b011010010110)

    def _pwm1_stop(self):
        self.timer1.write(0x00,0b000000010110)

    # level range from 0 to 8, 4 means no speed
    def speedL(self,level):
        self._pwm0(self.level2speed[level])
        self.nowStatus[0] = level

    # level range from 0 to 8, 4 means straight forward
    def steerL(self,level):
        self._pwm1(self.level2steer[level])
        self.nowStatus[1] = level

    def statusL(self,control_array):
        self._pwm0(self.level2speed[control_array[0]])
        self._pwm1(self.level2steer[control_array[1]])
        self.nowStatus[0] = control_array[0]
        self.nowStatus[1] = control_array[1]
        
    def stop(self):
        print("***Control Stop***")
        self._pwm0(self.level2speed[4])
        self._pwm1(self.level2steer[4])

    def statusAction(self, s):
        if s == 0:
            statusL([8,4])
        elif s == 1:
            statusL([6,2])
        elif s == 2:
            statusL([6,6])
        elif s == 3:
            stop()
