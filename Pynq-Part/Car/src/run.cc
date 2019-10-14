/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-09-19 12:44:06
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-14 17:07:22
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
#include <queue>
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
int commander = NNCONTROL;
mutex commanderLock;

mutex queueLock;
mutex controlLock;
time_t timeGet;
queue<Mat> takenImages;
queue<int> generatedCommands;
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

void setInputImage(DPUTask *task, const char* inNode, const cv::Mat& image) {
    DPUTensor* in = dpuGetInputTensor(task, inNode);
    //float scale = dpuGetTensorScale(in);
    //int width = dpuGetTensorWidth(in);
    //int height = dpuGetTensorHeight(in);
    //int size = dpuGetTensorSize(dpu_in);
    int8_t* data = dpuGetTensorAddress(in);
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < image.rows; ++j) {
            for(int k = 0; k < image.cols; ++k) {
               data[j*image.rows*3+k*3+2-i] = float(image.at<Vec3b>(j,k)[i])/255.0 - 0.5;
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

#define COMMANDMAXLEN 5
void addCommand(int com){
    controlLock.lock();
    int nowSize = generatedCommands.size();
    if( nowSize >= COMMANDMAXLEN){
        for(int i=0;i<nowSize;i++){
            generatedCommands.pop();
        }
    }
    generatedCommands.push(com);
    controlLock.unlock();
}

void run_model(DPUTask* task){
    int channel = kinds.size();
    vector<float> smRes(channel);
    int8_t *fcRes;
    Mat tmpImage;
    while (1)
    {
        queueLock.lock();
        if(takenImages.empty() || commander == CVCONTROL){
            queueLock.unlock();
            continue;
        }else{
            tmpImage = takenImages.front();
            takenImages.pop();
            time_t now = time(0);
            if(now < timeGet){
                queueLock.unlock();
                continue;
            }else{
                timeGet = now;
            }
        }
        queueLock.unlock();

        _T(setInputImage(task, CONV_INPUT_NODE, tmpImage));
        //dpuSetInputImage2(task,CONV_INPUT_NODE, tmpImage);
        _T(dpuRunTask(task));
        float scale = dpuGetOutputTensorScale(task, CONV_OUTPUT_NODE);
        DPUTensor *dpuOutTensor = dpuGetOutputTensor(task, CONV_OUTPUT_NODE);
        fcRes = dpuGetTensorAddress(dpuOutTensor);
        _T(dpuRunSoftmax(fcRes, smRes.data(), channel, 1, scale));
        //_T(TopK(smRes.data(),channel,4,kinds,"nowCap"));

        addCommand(topKind(smRes.data(), channel));        
    }
}

void run_cv(){
    if(mode == "nn")return;
    Mat tmpImage;
    while(true){
        queueLock.lock();
        if(takenImages.empty()){
            queueLock.unlock();
            continue;
        }else{
            tmpImage = takenImages.front();
            takenImages.pop();
            time_t now = time(0);
            if(now < timeGet){
                queueLock.unlock();
                continue;
            }else{
                timeGet = now;
            }
        }
        queueLock.unlock();
        if(commander == CVCONTROL){
		return;
        }
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
        queueLock.lock();
        int nowSize = takenImages.size();
        if(nowSize >= IMAGEMAXLEN){
            for(int i=0;i<nowSize;i++){
                takenImages.pop();
            }
        }
        takenImages.push(image);
        queueLock.unlock();
    }
    cap.release();
}

void run_command(){
    PYNQZ2 controller = PYNQZ2();
    while(true){
        controlLock.lock();
        if(generatedCommands.empty()){
            controlLock.unlock();
            continue;
        }
	cout<<"the command is:"<<generatedCommands.front()<<endl;
        controller.command(generatedCommands.front());
        generatedCommands.pop();
        controlLock.unlock();
    }
    }

int main(int argc, char **argv)
{
     if (argc != 2) {
          cout << "Usage of this exe: ./car cv/nn"
             << endl;
        return -1;
      }
    // nn means just use ml, cv means use ml & cv.
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
