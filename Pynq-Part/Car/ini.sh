### 
# @Author: Sauron Wu
 # @GitHub: wutianze
 # @Email: 1369130123qq@gmail.com
 # @Date: 2019-10-14 14:51:04
 # @LastEditors: Sauron Wu
 # @LastEditTime: 2019-10-28 17:19:06
 # @Description: must run in su mode
 ###
insmod ~/dpu.ko
xauth merge /home/xilinx/.Xauthority
mkdir src/build
# Then run collcet in su mode, or you will get Gtk Warning: cannot open display.
