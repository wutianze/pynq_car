/*
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-09-20 14:23:08
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-09-20 14:23:08
 * @Description: 
 */
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
queue<pair<int,Mat>> queueStore;
#define NOTSTART 0
#define STARTRECORD 1
#define EXIT 2
int startRecord = NOTSTART;
//mutex mutexshow;
string path = "./images/";
void storeImage()
{
    ofstream outFile;
    outFile.open(path + "train.csv", ios::out | ios::app);
    int count = 0;
    while(true){
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
            pair<int,Mat>tmpQ = queueStore.front();
	        queueStore.pop();
            mtxQueueStore.unlock();
            time_t now = time(0);
            string fileName = to_string(now) + to_string(count) + ".jpg";
   	        imwrite(path + fileName, tmpQ.second);
            outFile << fileName << ',';
            switch(tmpQ.first){
                case 0:
                outFile<< "1,0,0";
                break;
                case 1:
                outFile<<"0,1,0";
                break;
                case 2:
                outFile<<"0,0,1";
                break;
            }
            outFile<< endl;
            count = (count + 1)%100;
        }
            }
    outFile.close();
}

int main(int argc, char **argv)
{

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
        if (!controller.command(commandGet))
        {
            cout << "STOP ALL" << endl;
	        return 0;
        }
        mtxQueueStore.lock();
        queueStore.push(make_pair(commandGet,image));
        mtxQueueStore.unlock();
    }
    storeThread.join();
    destroyAllWindows();

    return 0;
}
