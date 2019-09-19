# pynq-auto
## Material
Pynq-Z2 with petalinux built image & pynq-z2-v2.4 image, for how to build please refer to [this](https://github.com/wutianze/dnndk3.0-pynqz2/blob/master/build-pynqz2-system.md), or you can just download the image we provided [here]().
## Preparation if you want to collect data in pynq-z2-v2.4(recommend)
### 1. Log in and run `sudo passwd root` to set your root password.
### 2. Run `sudo vi /etc/ssh/sshd_config` and enable `PermitRootLogin yes` & `StrictModes yes`.
### 3. Run `sudo service ssh restart` to restart ssh service.
### 4. Log in the board as root.
### 5. Run:
    ```shell
    apt-get update
    apt-get install network-manager
    ```
### 6. Reboot and run `nmtui`, now configure the wlan link and make sure you can log in the board using wireless network.
## Steps  
### 1. Copy the CarCollect directory to your Pynq
### 2. In Pynq `cd CarCollect/c++` and then run `make`, now you can run `./car` and use:
  > w for forward  
    s for stop  
    a for turn left  
    d for turn right  
    Esc for quit  
  >
  The process will collect the data used for training automatically. When you finish, you can see the directory images in current directory. The image name and labels are stored in train.csv.  
### 3. Copy the images to your host for train.  
### 4. Run `sudo python3 process_img.py` to do some pre work to images.   
If you change the labels number or define your own labels, please change the process_img function in process_img.py, the other parts can remain unchanged.  
### 5. Run `sudo python3 train.py` to do the train and you will get a h5 model.
### 6. Run the following command to transform .h5 model to .pb model.
    `python keras_to_tensorflow.py 
    --input_model="path/to/keras/model.h5" 
    --output_model="path/to/save/model.pb"`
### 7. Run `decent_q inspect --input_frozen_graph=path/to/model.pb` to see the name of your model's inputNode and outputNode.
### 8. Refer to [this](https://github.com/wutianze/dnndk3.0-pynqz2) to use decent tools in ./decent directory, finally you will get .elf in ./decent/compile/. Move the .elf file to CarRun directory.  
### 9. Copy the CarRun directory to Pynq.
### 10. In your Pynq board, edit the main.cc as you like and then `make` to get `car`.
### 11. Run the car.

