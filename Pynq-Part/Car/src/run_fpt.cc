/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-09-19 12:44:06
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-11-14 14:09:24
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
#include<csignal>
#include "cv_lane.h"
using namespace cv;
using namespace std;
using namespace std::chrono;

#define NNCONTROL 0
#define CVCONTROL 1
// commander indicates the car is controlled by AI or opencv currently
mutex commanderLock;
mutex exitLock;
mutex timeLock;
bool EXIT = false;
int commander = CVCONTROL;
clock_t timeSet;
safe_queue<Mat> takenImages;
struct steer_throttle_command{
    steer_throttle_command(){};
    steer_throttle_command(float s, float t):steer(s),throttle(t){};
    float steer;
    float throttle;
};
safe_queue<steer_throttle_command> generatedCommands;

#define KERNEL_CONV "testModel_0"
#define CONV_INPUT_NODE "conv2d_1_convolution"
#define CONV_OUTPUT_NODE "dense_3_MatMul"

#define TASKNUM 1
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

void sig_handler(int sig){
cout<<"SIGTSTP resceived\n";
if(sig == SIGTSTP){
	exitLock.lock();
	EXIT = true;
	exitLock.unlock();
}
}

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

#define COMMANDMAXLEN 3
void addCommand(steer_throttle_command tmpC){
    timeLock.lock();
    clock_t now = clock();
    if(now < timeSet){
        timeLock.unlock();
        return;
    }else{
    	//cout<<"FPS:"<<CLOCKS_PER_SEC/float(now-timeSet)<<endl;
        timeSet = now;
    }
    timeLock.unlock();
    int nowSize = generatedCommands.size();
    if( nowSize >= COMMANDMAXLEN){
        if(generatedCommands.try_pop())generatedCommands.push(tmpC);
        return;
    }else{
        generatedCommands.push(tmpC);
    }
}

void run_model(DPUTask* task){
    cout<<"Run Model\n";
    //int channel = kinds.size();
    int channel = 2;
    vector<float> smRes(channel);

    Mat tmpImage;
    while (1)
    {
	    exitLock.lock();
	    if(EXIT)break;
	    else{
	        exitLock.unlock();
	    }
        commanderLock.lock();
        if(commander == CVCONTROL){
            commanderLock.unlock();
            continue;
        }
        commanderLock.unlock();
        if(!takenImages.try_pop(tmpImage))continue;
	    //takenImages.wait_and_pop(tmpImage);
        _T(setInputImage(task, CONV_INPUT_NODE, tmpImage));
        //dpuSetInputImage2(task,CONV_INPUT_NODE, tmpImage);
        _T(dpuRunTask(task));
        float scale = dpuGetOutputTensorScale(task, CONV_OUTPUT_NODE);
        int8_t* modelRes = dpuGetTensorAddress(dpuGetOutputTensor(task, CONV_OUTPUT_NODE));
        steer_throttle_command tmpC = steer_throttle_command(((float*)smRes.data())[0],-1);
	    addCommand(tmpC);
    }
    exitLock.unlock();
    cout<<"Run Model Exit\n";
}

#define STRAIGHT_LEFT_K -7.5
#define STRAIGHT_RIGHT_K 7.5
#define K_RANGE 0.1
void run_cv(){
    cout<<"Run CV\n";
    Mat tmpImage;
    CvSize frame_size = cvSize(LANE_DET_WIDTH, LANE_DET_HEIGHT/2);
    IplImage *temp_frame = cvCreateImage(frame_size, IPL_DEPTH_8U, 3);
	IplImage *grey = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);
	IplImage *edges = cvCreateImage(frame_size, IPL_DEPTH_8U, 1);
	CvMemStorage* houghStorage = cvCreateMemStorage(0);

    while(true){
    	exitLock.lock();
    	if(EXIT)break;
    	else{
    	exitLock.unlock();
    	}
        if(!takenImages.try_pop(tmpImage))continue;
        IplImage frame_get = IplImage(tmpImage);
        int canFind = find_lane(&frame_get,temp_frame,grey,edges,houghStorage);
        
        steer_throttle_command tmpC;
        if(canFind == 1){//cannot find left but can find right
            float tmpK = laneR.k.get();
	    float tmpB = laneR.b.get();
	    if(tmpK > STRAIGHT_RIGHT_K + K_RANGE|| tmpK < 0)tmpC.steer = 0.5;
	    else if(tmpK < STRAIGHT_RIGHT_K - K_RANGE && tmpK > 0)tmpC.steer = float(STRAIGHT_RIGHT_K - K_RANGE - tmpK)/float(STRAIGHT_RIGHT_K - K_RANGE);
    	    if((72- tmpB)/tmpK < 140)tmpC.steer = -0.4;
	    cout<<"laneR k:"<<tmpK<<endl;
	    //cout<<"laneR b:"<<laneR.b.get()<<",k:"<<laneR.k.get()<<";steer:"<<tmpC.steer<<"\r\033[k";
        }else if(canFind == 3){
	    tmpC.steer = 0;
    	    //cout<<"cannot find any so steer:"<<tmpC.steer<<"\r\033[k";
	}else{
            float tmpB = laneL.b.get();
    	    float tmpK = laneL.k.get();
            if(tmpK > STRAIGHT_LEFT_K + K_RANGE && tmpK < 0)tmpC.steer = float(STRAIGHT_LEFT_K + K_RANGE - tmpK)/float(STRAIGHT_LEFT_K + K_RANGE);
            else if(tmpK < STRAIGHT_LEFT_K - K_RANGE || tmpK > 0)tmpC.steer = -0.5;
	    if((72- tmpB)/tmpK > 70)tmpC.steer = 0.4;
	    cout<<"laneL k:"<<tmpK<<endl;
    	    //if(tmpK > 10 && -tmpB/tmpK > TOO_LEFT)tmpC.steer = 
    	    //cout<<"laneL b:"<<laneL.b.get()<<",k:"<<laneL.k.get()<<"; laneR b:"<<laneR.b.get()<<",k:"<<laneR.k.get()<<";steer:"<<tmpC.steer<<"\r\033[k";
        }
	
	cout<<"steer:"<<setprecision(4)<<tmpC.steer<<endl;

        tmpC.throttle = -1.0;
        commanderLock.lock();
        if(commander == CVCONTROL){
            addCommand(tmpC);
            commanderLock.unlock();
            continue;
        }
        commanderLock.unlock();
    }
    exitLock.unlock();
    cvReleaseMemStorage(&houghStorage);

	cvReleaseImage(&grey);
	cvReleaseImage(&edges);
	cvReleaseImage(&temp_frame);
    cout<<"Run CV Exit\n";
}

#define IMAGEMAXLEN 10 
void run_camera(){
    cout<<"Run Camera\n";
    VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 160);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 120);
    Mat image;
    while(true){
	exitLock.lock();
	if(EXIT)break;
	else{
	exitLock.unlock();
	}
        cap >> image;
        int nowSize = takenImages.size();
        if(nowSize >= IMAGEMAXLEN){
            if(takenImages.try_pop())takenImages.push(image.rowRange(40,image.rows).clone());
        }else{
            takenImages.push(image.rowRange(40,image.rows).clone());
        }
    }
    exitLock.unlock();
    cout<<"Run Camera Exit\n";
    cap.release();
}

void run_command(){
    cout<<"Run Command\n";
    PYNQZ2 controller = PYNQZ2();
    //controller.throttleSet(runSpeed);
    while(true){
	exitLock.lock();
	if(EXIT){
		break;
		controller.steerSet(0.0);
		controller.throttleSet(0.0);
	}
	else{
	exitLock.unlock();
	}
        steer_throttle_command tmpS;
        if(!generatedCommands.try_pop(tmpS))continue;
    	controller.steerSet(tmpS.steer);
        controller.throttleSet(tmpS.throttle);
    }
    exitLock.unlock();
    cout<<"Run Steer Exit\n";
    }


int main(int argc, char **argv)
{
     if (argc != 1) {
        cout << "Usage of this exe: ./car"<< endl;
        return -1;
      }

    signal(SIGTSTP,sig_handler);
//
//    /* The main procress of using DPU kernel begin. */
//    DPUKernel *kernelConv;
//
//    dpuOpen();
//    kernelConv = dpuLoadKernel(KERNEL_CONV);
//    vector<DPUTask*> tasks(TASKNUM);
//    generate(tasks.begin(),tasks.end(),std::bind(dpuCreateTask,kernelConv,0));    
      vector<thread> threads;
//    for(int i=0;i<TASKNUM;i++){
//    	threads.push_back(thread(run_model,tasks[i]));
//    }

    threads.push_back(thread(run_command));
    threads.push_back(thread(run_camera));
    threads.push_back(thread(run_cv));
    for(int i = 0; i < threads.size(); i++){
        threads[i].join();
        cout<<"one exit:"<<i<<endl;
    }

    //for_each(tasks.begin(),tasks.end(),dpuDestroyTask);

    //dpuDestroyKernel(kernelConv);
    //dpuClose();
    return 0;
}
