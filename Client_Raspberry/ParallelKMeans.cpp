#include "ParallelKMeans.h"

using namespace std;

void compute_histogram(int numberpoints, struct mycentroid * points, int *his){
	
	for(int i=0;i<number_centers;i++)
		his[i] =0;
	for(int i=0; i< numberpoints; i++){
		his[points[i].cluster]++;
	}
}
void accumulatedDistanceParallel(int numberpoints, struct mycentroid * points,struct mycentroid * centroids) {
	#pragma omp parallel for
	for(int i=0; i< number_centers; i++){
		for(int j=0; j< Dimensions; j++){
			centroids[i].values[j] = 0;
		}
		centroids[i].cluster = 0;//the number of count
		
	}
	
	for(int i=0; i< numberpoints; i++){
		int ww = points[i].cluster;
		for(int j=0; j< Dimensions;j++){
			centroids[ww].values[j] += points[i].values[j];
		}
		centroids[ww].cluster++;
	}
}

//compute distance between centroid and point
double mydistance(struct mycentroid p1,struct mycentroid p2){
	double result=0.0;
	for (int i=0; i< Dimensions; i++){
		result += (p1.values[i]-p2.values[i])*(p1.values[i]-p2.values[i]);
		//result += pow(p1.values[i]-p2.values[i], 2);
	}
	return sqrt(result);

}


int assignPointsToNearestClusterParallel(int numberpoints, struct mycentroid * points,struct mycentroid * centroids) {
	double minDistance;
	int bestCluster;
	int numChanged = 0;
	double dist;
	
	
	//#pragma omp parallel for num_threads(num_omp_threads) private(minDistance, bestCluster, dist)
	#pragma omp parallel for private(minDistance, bestCluster, dist)
	for (int i = 0; i < numberpoints; i++) {
		minDistance = mydistance(centroids[0], points[i]);
		bestCluster = 0;
		for (int j = 1; j < number_centers; j++) {
			dist = mydistance(centroids[j], points[i]);
			if (dist < minDistance) {
				minDistance = dist;
				bestCluster = j;
			}
		}
        //#pragma omp critical
        omp_set_lock(&lock); 
		if (points[i].cluster != bestCluster) {
			points[i].cluster = bestCluster;
			numChanged++;
		}
		omp_unset_lock(&lock);
	}
	return numChanged;
}
