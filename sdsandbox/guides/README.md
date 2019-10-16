<!--
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-09-23 10:12:28
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-16 09:42:54
 * @Description: 
 -->
# Introduction
The simulator we provided is based on the sdsandbox project. For more information about the initial project, please refer to [this](https://github.com/tawnkramer/sdsandbox). 

# What you can do with the simulator
- Collect training data
- Test your model or algorithms in the virtual world
- Play computer games only(not recommend)

## Setup

You need to have [Unity](https://unity3d.com/get-unity/download) installed, and python3+, tensorflow, keras is also needed.

Linux Unity install [here](https://forum.unity3d.com/threads/unity-on-linux-release-notes-and-known-issues.350256/). Check last post in this thread.

## Using Unity3d to test your .h5 model

1) Start the prediction server with the pre-trained model. 

```bash
cd sdsandbox
python src/predict_server.py path-to-your-model/model.h5
```
 If you get a crash loading this model, you will not be able to run the demo. But you can still generate your own model. This is a problem between tensorflow/keras versions. 
 
2) Load the Unity project sdsandbox/sdsim in Unity. Double click on Assets/Scenes/main to open that scene.  

3) Hit the start button to launch. Then "Use NN Steering".  

## Generate training data

1) Load the Unity project sdsandbox/sdsim in Unity.  

2) Create a dir sdsandbox/sdsim/log.  

3) Hit the start arrow in Unity to launch project.  

4) Hit button "Generate Training Data" to generate image and steering training data. See sdsim/log for output files.  

5) Stop Unity sim by clicking run arrow again.  

6) Run this python script to prepare raw data for training:  

```bash
cd sdsandbox
python src/prepare_data.py --src=sdsim/log/*.* --dest=dataset
```

7) Repeat 4, 5, 6 until you have lots of training data. Over 50K images for best results.

## How to edit Unity3d to use your own model
1. The Control commands can be changed in JoystickCarControl.cs
2. The training data style can be changed in Logger.cs
3. You should enable the hidden buttons in Canvas/Panel Menu like ManualTraingData
4. The tcp control can be changed in Script/tcp/TcpCarHandler.cs, and you need to change the predict_server.py too


