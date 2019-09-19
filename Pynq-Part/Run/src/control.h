#include<stdio.h>
#include<fcntl.h>
#include<sys/mman.h>
using namespace std;
class Controller{
    public:
    virtual void forward() = 0;
    virtual void left() = 0;
    virtual void right() = 0;
    virtual void stop() = 0;   
    virtual bool command(int) = 0;
};
class PYNQZ2: public Controller{
    private:
    int* throttle;
    int* steer;
    char* leds;

    public:
    int status;
    PYNQZ2();
    ~PYNQZ2();
    void forward();
    void left();
    void right();
    void stop();
    bool command(int);

    void throttleSet(int);
    void steerSet(int);

};