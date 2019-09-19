/*
-- (c) Copyright 2018 Xilinx, Inc. All rights reserved.
--
-- This file contains confidential and proprietary information
-- of Xilinx, Inc. and is protected under U.S. and
-- international copyright and other intellectual property
-- laws.
--
-- DISCLAIMER
-- This disclaimer is not a license and does not grant any
-- rights to the materials distributed herewith. Except as
-- otherwise provided in a valid license issued to you by
-- Xilinx, and to the maximum extent permitted by applicable
-- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
-- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
-- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
-- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
-- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
-- (2) Xilinx shall not be liable (whether in contract or tort,
-- including negligence, or under any other theory of
-- liability) for any loss or damage of any kind or nature
-- related to, arising under or in connection with these
-- materials, including for any direct, or any indirect,
-- special, incidental, or consequential loss or damage
-- (including loss of data, profits, goodwill, or any type of
-- loss or damage suffered as a result of any action brought
-- by a third party) even if such damage or loss was
-- reasonably foreseeable or Xilinx had been advised of the
-- possibility of the same.
--
-- CRITICAL APPLICATIONS
-- Xilinx products are not designed or intended to be fail-
-- safe, or for use in any application requiring fail-safe
-- performance, such as life-support or safety devices or
-- systems, Class III medical devices, nuclear facilities,
-- applications related to the deployment of airbags, or any
-- other applications that could lead to death, personal
-- injury, or severe property or environmental damage
-- (individually and collectively, "Critical
-- Applications"). Customer assumes the sole risk and
-- liability of any use of Xilinx products in Critical
-- Applications, subject only to applicable laws and
-- regulations governing limitations on product liability.
--
-- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
-- PART OF THIS FILE AT ALL TIMES.
*/
/*#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <atomic>
#include <sys/stat.h>
#include <unistd.h>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include <dnndk/dnndk.h>
#include <opencv2/opencv.hpp>
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

string model;

#define NNCONTROL 0
#define CVCONTROL 1
int commander = NNCONTROL;
mutex commanderLock;

mutex queueLock;
mutex controlLock;
time_t timeGet;
queue<Mat> takenImages;
queue<int> generatedCommands;
vector<string> kinds = {"left", "forward", "right", "stop"};
#define KERNEL_CONV "mnist_0"
#define CONV_INPUT_NODE "conv2d_1_convolution"
#define CONV_OUTPUT_NODE "dense_2_MatMul"
#define TASKNUM 3

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

void setInputImage(DPUTask *task, const string& inNode, const cv::Mat& image) {
    DPUTensor* in = dpuGetInputTensor(task, inNode);
    //float scale = dpuGetTensorScale(in);
    //int width = dpuGetTensorWidth(in);
    //int height = dpuGetTensorHeight(in);
    //int size = dpuGetTensorSize(dpu_in);
    int8_t* data = dpuGetTensorAddress(in);
    for(int i = 0; i < 3; ++i) {
        for(int j = 0; j < image.rows; ++j) {
            for(int k = 0; k < image.cols; ++k) {
               data[j*image.rows*3+k*3+2-i] = float(image.at<Vec3b>(j,k)[i])/255.0;
            }
        }
    }
}

int command(const float *d, int size)
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
            tmpImage = takenImages.top();
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
        //Mat image = imread("tmp.jpg",1);
        //Scalar mM = mean(tmpImage);
        //float meanV[3] = {mM.val[0], mM.val[1], mM.val[2]};

        _T(setInputImage(task, CONV_INPUT_NODE, tmpImage));
        _T(dpuRunTask(task));
        float scale = dpuGetOutputTensorScale(task, CONV_OUTPUT_NODE);
        DPUTensor *dpuOutTensor = dpuGetOutputTensor(task, CONV_OUTPUT_NODE);
        fcRes = dpuGetTensorAddress(dpuOutTensor);
        _T(dpuRunSoftmax(fcRes, smRes.data(), channel, 1, scale));
        //_T(TopK(smRes.data(),channel,4,kinds,"nowCap"));
        int toDo = command(smRes.data(), channel);
        addCommand(toDo);        
    }
}

void run_cv(){
    if(model == "nn")return;
    Mat tmpImage;
    while(true){
        queueLock.lock();
        if(takenImages.empty()){
            queueLock.unlock();
            continue;
        }else{
            tmpImage = takenImages.top();
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
        //commander = bool judgeSituation()
        if(commander == CVCONTROL){

        }
    }
}

#define IMAGEMAXLEN 5
void run_Camera(){
    VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 160);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 120);
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

void run_Command(){
    PYNQZ2 controller = PYNQZ2();
    while(true){
        controlLock.lock();
        if(generatedCommands.empty()){
            controlLock.unlock();
            continue;
        }
        controller.command(generatedCommands.top());
        generatedCommands.pop();
        controlLock.unlock();
    }
    }

int main(int argc, char **argv)
{
     if (argc != 3) {
          cout << "Usage of this exe: ./car cv/nn"
             << endl;
        return -1;
      }
    // nn means just use ml, cv means use ml & cv.
    model = argv[1];

    /* The main procress of using DPU kernel begin. */
    DPUKernel *kernelConv;

    TRDWarning();

    dpuOpen();
    // Create the kernel for mnist
    kernelConv = dpuLoadKernel(KERNEL_CONV);
    vector<DPUTask*> tasks(TASKNUM);
    generate(tasks.begin(),task.end(),std::bind(dpuCreateTask,kernelConv,0));    
    //DPUTask *taskMnist = dpuCreateTask(kernelConv, 0);
    int threadNum = TASKNUM + 3;
    array<thread,threadNum> threads = {
        thread(run_model, tasks[0]),
        thread(run_model, tasks[1]),
        thread(run_model, tasks[2]),
        thread(run_command),
        thread(run_camera),
        thread(run_cv)
    };
    for(int i = 0; i < threadNum; i++){
        threads[i].join();
    }
    for_each(tasks.begin(),tasks.end(),dpuDestroyTask);

    dpuDestroyKernel(kernelConv);
    dpuClose();
    return 0;
}
