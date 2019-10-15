### 
# @Author: Sauron Wu
 # @GitHub: wutianze
 # @Email: 1369130123qq@gmail.com
 # @Date: 2019-09-25 15:32:42
 # @LastEditors: Sauron Wu
 # @LastEditTime: 2019-10-15 14:16:29
 # @Description: 
 ###

rm -rf ./training_data_npz
python process_img.py --path=/home/sauron/pynq_car/sdsandbox/sdsim/lsr-pid2 --store=./training_data_npz --method=0

rm -rf ./model_stored
python train.py --model=./model_stored --read=./training_data_npz



