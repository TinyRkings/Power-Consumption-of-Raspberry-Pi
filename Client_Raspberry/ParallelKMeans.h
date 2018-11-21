#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <omp.h>
#include <iostream> // for standard I/O
#include <string>   // for strings
#include <iomanip>  // for controlling float print precision
#include <sstream>  // string to number conversion
#include <opencv2/core.hpp>     // Basic OpenCV structures (cv::Mat, Scalar)
#include <opencv2/imgproc.hpp>  // Gaussian Blur
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>  // OpenCV window I/O
#include <opencv2/imgcodecs.hpp>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#define number_centers 25
//#define Dimensions 2262
#define Dimensions 288
#define num_omp_threads 4
static omp_lock_t lock;
using namespace std;

struct mycentroid{
	int values[Dimensions];
	int cluster;
};

//#define Dimensions 288 = 18* 4* 4
struct HSV{
	float H;
	float S;
	float V;
};

//int initPointsParallel(string file, int numberpoints, struct mycentroid* points);
void accumulatedDistanceParallel(int numberpoints, struct mycentroid * points,struct mycentroid * centroids);
double mydistance(struct mycentroid p1,struct mycentroid p2);
int assignPointsToNearestClusterParallel(int numberpoints, struct mycentroid * points,struct mycentroid * centroids);
void compute_histogram(int numberpoints, struct mycentroid * points, int *his);

//void initPoints(int number_points, struct mycentroid *points);
int Cal_bucket(struct HSV hsv_value);
HSV convertBGRtoHSV(int B, int G, int R );
int getMinVal(int R, int G, int B);
int getMaxVal(int R, int G, int B);

int Current_Power(int file);
