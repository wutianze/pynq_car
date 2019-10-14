/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-10-14 09:10:53
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-14 11:22:32
 * @Description: 
 */
#include<stdio.h>
#include<fcntl.h>
#include<sys/mman.h>
#include<string>
using namespace std;
class Controller{
    public: 
    virtual string to_record() = 0;
};
class PYNQZ2: public Controller{
    private:
    int* throttle;
    int* steer;
    char* leds;
    void throttleSet(float);
    void steerSet(float);
    void forward();
    void left();
    void right();
    void stop();
    void faster();
    void slower();
    void lefter();
    void righter();
    
    public:
    struct Status{
        float throttleRate;
        float steerRate;
    };
    Status* nowS;
    PYNQZ2();
    ~PYNQZ2();
    Status* command(int);
    string to_record();
};
