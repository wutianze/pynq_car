/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-09-19 12:44:06
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-21 17:51:30
 * @Description: 
 */
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <dnndk/dnndk.h>
#include <opencv2/opencv.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <sstream>
#include "safe_queue.h"
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include "control.h"
using namespace cv;
using namespace std;
using namespace std::chrono;

//whether to use opencv, set by user
string mode;

#define NNCONTROL 0
#define CVCONTROL 1
// commander indicates the car is controlled by AI or opencv currently
mutex commanderLock;
mutex timeLock;
int commander = NNCONTROL;
time_t timeSet;
safe_queue<Mat> takenImages;
safe_queue<int> generatedCommands;
//vector<string> kinds = {"left", "forward", "right", "stop"};
vector<string> kinds = {"steer"};

#define KERNEL_CONV "testModel"
#define CONV_INPUT_NODE "conv2d_1_convolution"
#define CONV_OUTPUT_NODE "dense_2_MatMul"

#define TASKNUM 3
#define THREADNUM 6 //THREADNUM should be TASKNUM + 3

//#define SHOWTIME
#ifdef SHOWTIME
#define _T(func)                                                              \
    {                                                                         \
        auto _start = system_clock::now();                                    \
        func;                                                                 \
        auto _end = system_clock::now();                                      \
        auto duration = (duration_cast<microseconds>(_end - _start)).count(); \
        string tmp = #func;                                                   \
        tmp = tmp.substr(0, tmp.find('('));                                   \
        cout << "[TimeTest]" << left << setw(30) << tmp;                      \
        cout << left << setw(10) << duration << "us" << endl;                 \
    }
#else
#define _T(func) func;
#endif

// the input image is in BGR format(opencv default), and the '/255 - 0.5' must be the same when you 
// train the model, don't forget to multiply the scale finally
void setInputImage(DPUTask *task, const char* inNode, const cv::Mat& image) {
    DPUTensor* in = dpuGetInputTensor(task, inNode);
    float scale = dpuGetTensorScale(in);
    int8_t* data = dpuGetTensorAddress(in);
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < image.rows; ++j) {
            for(int k = 0; k < image.cols; ++k) {
               data[j*image.rows*3+k*3+i] = (float(image.at<Vec3b>(j,k)[i])/255.0 - 0.5)*scale;
            }
        }
    }
}

int topKind(const float *d, int size)
{
    assert(d && size > 0);
    int result = 0;
    float maxx = d[0];
    for (auto i = 0; i < size; ++i)
    {
        if (d[i] > maxx)
        {
            maxx = d[i];
            result = i;
        }
    }
    return result;
}

#define COMMANDMAXLEN 3
void addCommand(int com){
    timeLock.lock();
    time_t now = time(0);
    if(now < timeSet){
        timeLock.unlock();
        return;
    }else{
        timeSet = now;
    }
    timeLock.unlock();
    int nowSize = generatedCommands.size();
    if( nowSize >= COMMANDMAXLEN){
        if(generatedCommands.try_pop())generatedCommands.push(com);
        return;
    }else{
        generatedCommands.push(com);
    }
}

void run_model(DPUTask* task){
    int channel = kinds.size();
    vector<float> smRes(channel);
    int8_t *fcRes;
    Mat tmpImage;
    while (1)
    {
        commanderLock.lock();
        if(commander == CVCONTROL){
            commanderLock.unlock();
            continue;
        }
        commanderLock.unlock();
        takenImages.wait_and_pop(tmpImage);
        _T(setInputImage(task, CONV_INPUT_NODE, tmpImage));
        //dpuSetInputImage2(task,CONV_INPUT_NODE, tmpImage);
        _T(dpuRunTask(task));
        float scale = dpuGetOutputTensorScale(task, CONV_OUTPUT_NODE);
        modelRes = dpuGetTensorAddress(dpuGetOutputTensor(task, CONV_OUTPUT_NODE));
        _T(dpuRunSoftmax(modelRes, smRes.data(), channel, 1, scale));

        addCommand(topKind(smRes.data(), channel));        
    }
}

int cv_al1(Mat image){
    return 0;
}

void run_cv(){
    if(mode[0] == 'n')return;
    Mat tmpImage;
    while(true){
        takenImages.wait_and_pop(tmpImage);
        int tmpCommand = cv_al1(tmpImage);
        if(tmpCommand == 0)continue;
        commanderLock.lock();
        if(commander == CVCONTROL){
            commanderLock.unlock();
            continue;
        }
        commanderLock.unlock();
    }
}

#define IMAGEMAXLEN 5
void run_camera(){
    VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 160);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 80);
    Mat image;
    while(true){
        cap >> image;
        int nowSize = takenImages.size();
        if(nowSize >= IMAGEMAXLEN){
            if(takenImages.try_pop())takenImages.push(image);
        }else{
            takenImages.push(image);
        }
    }
    cap.release();
}

void run_command(){
    PYNQZ2 controller = PYNQZ2();
    controller.throttleSet(0.5);
    while(true){
        int tmpC;
        generatedCommands.wait_and_pop(tmpC);
        cout<<"the command is:"<<tmpC<<endl;
        switch(tmpC){
            case 0:
            controller.steerSet(-0.5);
            break;
            case 1:
            controller.steerSet(0);
            break;
            case 2:
            controller.steerSet(0.5);
            break;
        }
    }
    }

int main(int argc, char **argv)
{
     if (argc != 2) {
        cout << "Usage of this exe: ./car c/n"<< endl;
        return -1;
      }
    // n means just use ml, c means use ml & cv.
    mode = argv[1];
    /* The main procress of using DPU kernel begin. */
    DPUKernel *kernelConv;

    dpuOpen();
    kernelConv = dpuLoadKernel(KERNEL_CONV);
    vector<DPUTask*> tasks(TASKNUM);
    generate(tasks.begin(),tasks.end(),std::bind(dpuCreateTask,kernelConv,0));    
    array<thread,THREADNUM> threads = {
        thread(run_model, tasks[0]),
        thread(run_model, tasks[1]),
        thread(run_model, tasks[2]),
        thread(run_command),
        thread(run_camera),
        thread(run_cv)
    };

    for(int i = 0; i < THREADNUM; i++){
        threads[i].join();
        cout<<"one exit"<<endl;
    }
    for_each(tasks.begin(),tasks.end(),dpuDestroyTask);

    dpuDestroyKernel(kernelConv);
    dpuClose();
    return 0;
}
