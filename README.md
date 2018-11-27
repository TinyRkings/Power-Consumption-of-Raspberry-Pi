# Power-Consumption-of-Raspberry-Pi
Online Power measurement of Raspberry Pi 3

# Raspberry Pi3 Model B:

(1) install Raspbian (NOOBS)

(2) install opencv3:

cmake -D CMAKE_BUILD_TYPE=Release/Debug -D CMAKE_INSTALL_PREFIX=/home/pi/Documents/software/opencv-4.0.0 ..


cmake -D CMAKE_BUILD_TYPE=Release/Debug -D CMAKE_INSTALL_PREFIX=/home/lou.125/software/opencv-3.4.1 -D OPENCV_EXTRA_MODULES_PATH=/home/lou.125/software/opencv-3.4.1/modules ..

raspberry install opencv3

(3) Enable videocapture in Raspberry Pi 3 Model B:

a.sudo apt-get update  sudo apt-get upgrade

b. sudo raspi-config

c. sudo modprobe bcm2835-v4l2


# Run Command:

(1) Server:

g++-8 Server.cpp Server_HsvOperations.cpp Server_KMeans.cpp Server_KMeans.h Server_socket.cpp Server_socket.h -o Server -fopenmp


(2) Client_Raspberry:

g++ Main1.cpp HsvOperations.cpp ParallelKMeans.cpp ParallelKMeans.h ADS1115.cpp Client.cpp Client.h `pkg-config opencv --cflags --libs` -fopenmp -o node1
