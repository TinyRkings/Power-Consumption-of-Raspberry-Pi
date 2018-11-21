#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fstream>
#include <omp.h>
#include <iostream> // for standard I/O
#include <string>   // for strings
#include <iomanip>  // for controlling float print precision
#include <sstream>  // string to number conversion

#define number_centers 25
//#define Dimensions 2262
#define Dimensions 288
#define number_points 180*1 //300* Queue_Line
#define num_omp_threads 4
#define sampling 12
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

void initcentroidsParallel(struct mycentroid* centroids);
void UpdateCentroids(int numberpoints, struct mycentroid * points,struct mycentroid * centroids);
double mydistance(struct mycentroid p1,struct mycentroid p2);
double mydistance2(struct mycentroid p1,struct mycentroid*p2);
int assignPointsToNearestClusterParallel(int numberpoints, struct mycentroid * points,struct mycentroid * centroids);
void compute_histogram(int numberpoints, struct mycentroid * points, int *his);

int Cal_bucket(struct HSV hsv_value);
HSV convertBGRtoHSV(int B, int G, int R );
int getMinVal(int R, int G, int B);
int getMaxVal(int R, int G, int B);
