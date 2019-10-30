### 
# @Author: Sauron Wu
 # @GitHub: wutianze
 # @Email: 1369130123qq@gmail.com
 # @Date: 2019-10-14 14:51:04
 # @LastEditors: Please set LastEditors
 # @LastEditTime: 2019-10-30 10:37:43
 # @Description: must run in su mode
 ###
insmod /home/xilinx/dpu.ko
xauth merge /home/xilinx/.Xauthority
mkdir src/build
# Then run collcet in su mode, or you will get Gtk Warning: cannot open display.