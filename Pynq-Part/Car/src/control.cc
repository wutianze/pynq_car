/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-10-14 09:10:53
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-14 11:35:09
 * @Description: 
 */
#include"control.h"
#define THROTTLEMAX 500000
#define THROTTLEZERO 140000
#define STEERMAX 2000000
#define STEERZERO 100000
PYNQZ2::PYNQZ2(){
    int fd = open("/dev/mem",O_RDWR);
    
    throttle = (int*)mmap(0, 30, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x82800000);
    throttle[4] = 0b001010010110;
    throttle[0] = 0b011010010110;
    throttle[1] = THROTTLEMAX;
    throttleSet(0);

    steer = (int*)mmap(0, 30, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x82810000);
    steer[4] = 0b001010010110;
    steer[0] = 0b011010010110;
    steer[1] = STEERMAX;
    steerSet(0);

    leds = (char*)mmap(0, 8, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x81210000);
    leds[4] = 0x00;
    leds[0] = 0x0F;
}
PYNQZ2::STATUS nowStatus(){
    return nowS;
}
void PYNQZ2::throttleSet(float rate){
    rate = (rate > 1.0?1.0:rate) < (-1.0)?(-1.0):rate;
    throttle[5] = THROTTLEZERO + rate * (THROTTLEMAX - THROTTLEZERO);
    nowS.throttleRate = rate;
}

void PYNQZ2::steerSet(float rate){
    rate = (rate > 1.0?1.0:rate) < (-1.0)?(-1.0):rate;
    steer[5] = STEERZERO + rate * (STEERMAX - STEERZERO);
    nowS.steerRate = rate;
}

void PYNQZ2::forward(){
    throttleSet(0.5);
    steerSet(0);
    leds[0] = 0x01;
}

void PYNQZ2::left(){
    throttleSet(0.3);
    steerSet(-0.5);
    leds[0] = 0x02;
}

void PYNQZ2::right(){
    throttleSet(0.3);
    steerSet(0.5);
    leds[0] = 0x04;
}

void PYNQZ2::stop(){
    throttleSet(0);
    steerSet(0);
    leds[0] = 0x00;
}

void PYNQZ2::faster(){
    throttleSet(nowS.throttleRate+0.1);
}

void PYNQZ2::slower(){
    throttleSet(nowS.throttleRate-0.1);
}

void PYNQZ2::lefter(){
    steerSet(nowS.steerRate-0.1);
}

void PYNQZ2::righter(){
    steerSet(nowS.steerRate+0.1);
}

PYNQZ2::STATUS PYNQZ2::command(int c){
    switch(c){
        case 0:left();break;
        case 1:forward();break;
        case 2:right();break;
        case 3:stop();break;
        case 4:faster();break;
        case 5:slower();break;
        case 6:lefter();break;
        case 7:righter();break;
        default:stop();return NULL;
    }
    return nowS;
}

string PYNQZ2::to_record(){
    return to_string(nowS.steerRate) + ',' + to_string(nowS.throttleRate);
}

PYNQZ2::~PYNQZ2(){
    throttle[0] = 0b000000010110;
    steer[0] = 0b000000010110;
    leds[0] = 0x00;
}