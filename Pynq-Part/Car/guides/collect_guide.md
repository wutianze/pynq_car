<!--
 * @Author: Sauron Wu
 * @GitHub: wutianze
 * @Email: 1369130123qq@gmail.com
 * @Date: 2019-10-14 14:57:47
 * @LastEditors: Sauron Wu
 * @LastEditTime: 2019-10-14 14:57:47
 * @Description: 
 -->
# What you will learn:
- How to collect training data.
- How to define your own controller.

# How to use
## Step 1:
Do some preparations. ini.sh will help you load the dpu.ko and set X11.
```shell
cd ~/Car
su
chmod +x ./ini.sh
./ini.sh
```

## Step2:
Compile the code and try.
```shell
make collect
./build/collect 50000 # 50000 means how many pictures you need to collect
```
Now you will see a small window in your PC and you can use your keyboard to control your car:
- w: move forward
- a: turn left
- d: turn right
- s: stop
- t: start to save images until the number of pictures reach the threshold you set before
- Esc: stop the whole process
The pictures and labels are saved in the directory `images`.

# Define your own controller
## 1. Modify the labels you want to save
In `src/control.h&control.cc`, you can see the current controller class PYNQZ2, the function `to_record` will return a string which will be written as labels. PYNQZ2 controller will store the throttle value and steer value of the car currently. You can replace the function content by your own to store other information.  
***Tip: The label file's format is csv which means values should be separated by ','***
## 2. Modify the keyboard settings
You can choose a more comfortable way to play the car. The function you need to change in this part is in `collect.cc/main`. You will see a `switch(c)` in `main`, read the code and I'm sure you can define your own settings quickly.  
***Tip: The key-command mapping table is in src/controll.cc/command()***
