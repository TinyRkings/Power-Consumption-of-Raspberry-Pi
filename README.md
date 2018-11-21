# Power-Consumption-of-Raspberry-Pi
Online Power measurement of Raspberry Pi 3

Enable videocapture in Raspberry Pi 3 Model B:

sudo modprobe bcm2835-v4l2

Server:

g++-8 Server.cpp Server_HsvOperations.cpp Server_KMeans.cpp Server_KMeans.h Server_socket.cpp Server_socket.h -o Server -fopenmp


Client_Raspberry:

g++ Main1.cpp HsvOperations.cpp ParallelKMeans.cpp ParallelKMeans.h ADS1115.cpp Client.cpp Client.h `pkg-config opencv --cflags --libs` -fopenmp -o node1
