### 
# @Author: Sauron Wu
 # @GitHub: wutianze
 # @Email: 1369130123qq@gmail.com
 # @Date: 2019-10-14 14:51:04
 # @LastEditors: Sauron Wu
 # @LastEditTime: 2019-10-28 17:51:08
 # @Description: Must run in su mode and after running, the program should be run in su mode, or you will get Gtk Warning: cannot open display.

 ###
insmod ~/dpu.ko
xauth merge /home/xilinx/.Xauthority
mkdir src/build