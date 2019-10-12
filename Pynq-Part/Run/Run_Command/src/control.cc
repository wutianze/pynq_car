#include"control.h"
PYNQZ2::PYNQZ2(){
    status = 0;
    int fd = open("/dev/mem",O_RDWR);
    
    throttle = (int*)mmap(0, 30, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x82800000);
    throttle[4] = 0b001010010110;
    throttle[0] = 0b011010010110;
    throttle[1] = 500000;
    throttle[5] = 140000;

    steer = (int*)mmap(0, 30, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x82810000);
    steer[4] = 0b001010010110;
    steer[0] = 0b011010010110;
    steer[1] = 2000000;
    steer[5] = 100000;

    leds = (char*)mmap(0, 8, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0x81210000);
    leds[4] = 0x00;
    leds[0] = 0x0F;
}
void PYNQZ2::forward(){
    throttle[5] = 175000;
    steer[5] = 100000;
    status = 1;
    leds[0] = 0x01;
}

void PYNQZ2::left(){
    throttle[5] = 160000;
    steer[5] = 70000;
    status = 0;
    leds[0] = 0x02;
}

void PYNQZ2::right(){
    throttle[5] = 160000;
    steer[5] = 130000;
    status = 2;
    leds[0] = 0x04;
}

void PYNQZ2::stop(){
    throttle[5] = 140000;
    steer[5] = 100000;
    status = 3;
    leds[0] = 0x00;
}

bool PYNQZ2::command(int c){
    if(c == status)return true;
    switch(c){
        case 0:left();break;
        case 1:forward();break;
        case 2:right();break;
        case 3:stop();break;
        default:stop();return false;
    }
    return true;
}

void PYNQZ2::throttleSet(int value){
    throttle[5] = value;
}

void PYNQZ2::steerSet(int value){
    steer[5] = value;
}


PYNQZ2::~PYNQZ2(){
    throttle[0] = 0b000000010110;
    steer[0] = 0b000000010110;
    leds[0] = 0x00;
}