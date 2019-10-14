/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-09-20 14:23:08
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-14 14:50:39
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
#include <opencv2/opencv.hpp>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <sstream>
#include <thread>
#include <queue>
#include <mutex>
#include "control.h"
using namespace cv;
using namespace std;
using namespace std::chrono;

//int threadnum;
mutex mtxQueueStore;
queue<pair<string,Mat>> queueStore;
#define NOTSTART 0
#define STARTRECORD 1
#define EXIT 2

int imgMax = 50000;
int startRecord = NOTSTART;
//mutex mutexshow;
string path = "./images/";

void storeImage()
{
    ofstream outFile;
    outFile.open(path + "train.csv", ios::out | ios::app);
    int count = 0;
    while(count <= imgMax){
        sleep(0.2);
	    mtxQueueStore.lock();
        if(startRecord == EXIT){
            mtxQueueStore.unlock();
            break;
        }else if(startRecord == NOTSTART){
            mtxQueueStore.unlock();
            continue;
        }
        if(queueStore.empty()){
            mtxQueueStore.unlock();
        }else{
            pair<string,Mat>tmpQ = queueStore.front();
	        queueStore.pop();
            mtxQueueStore.unlock();
            time_t now = time(0);
            string fileName = to_string(now) + to_string(count) + ".jpg";
   	        imwrite(path + fileName, tmpQ.second);
            outFile << fileName << ','<< tmpQ.first <<endl;
            count++;
        }
            }
    outFile.close();
}

int main(int argc, char **argv)
{
    if (argc != 2) {
          cout << "Usage of this exe: ./collect 50000"
             << endl;
        return -1;
      }
    // nn means just use ml, cv means use ml & cv.
    imgMax = atoi(argv[1]);
    PYNQZ2 controller = PYNQZ2();

    int commandGet = 3;
    VideoCapture cap(0);
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 160);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 120);
    Mat image;
    if (access(path.c_str(), 0) == -1)
    {
        mkdir(path.c_str(), 0777);
    }
    thread storeThread = thread(storeImage);
    while (startRecord != EXIT)
    {
        cap >> image;
        imshow("car see", image);
        char c = (char)waitKey(1);
        switch (c)
        {
        case 'w':
            commandGet = 1;
            cout << c << endl;
            break;
        case 'a':
            commandGet = 0;
            cout << c << endl;
            break;
        case 's':
            commandGet = 3;
            cout << c << endl;
            break;
        case 'd':
            commandGet = 2;
            cout << c << endl;
            break;
	    case 't':
	        startRecord = STARTRECORD;
	        cout<<"Start Record\n";
	        break;
        case 27:
            startRecord = EXIT;
            commandGet = 3;
        }
        PYNQZ2::Status* status = controller.command(commandGet);
        if (status == NULL)
        {
            cout << "STOP ALL" << endl;
	        return -1;
        }
        string oneRecord = controller.to_record();
        mtxQueueStore.lock();
        queueStore.push(make_pair(oneRecord,image));
        mtxQueueStore.unlock();
    }
    storeThread.join();
    destroyAllWindows();

    return 0;
}
