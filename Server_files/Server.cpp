#include "Server_KMeans.h"
#include "Server_socket.h"

using namespace std;


//return the closet centroid number to the testpoints
int closet_centroid(struct mycentroid testpoints, struct mycentroid*centroids){
	
	double dist;
	double minDistance = mydistance2(centroids[0], &testpoints);
	int bestCluster = 0;
	for (int j = 1; j < number_centers; j++) {
			dist = mydistance2(centroids[j], &testpoints);
			if (dist < minDistance) {
				minDistance = dist;
				bestCluster = j;
			}
	}
	return bestCluster;
}

// return the node number which has most elements in this bucket
int most_bucket(int cluster_number,int his[QUEUE_LINE][number_centers]){
	int most_size= his[0][cluster_number];
	int result = 0;
	for(int i=1;i<QUEUE_LINE;i++){
		if(his[i][cluster_number]>most_size)
		{
			most_size = his[i][cluster_number];
			result = i;
		}
	}
	return result;
		
}

int one_image_uchar(uchar *image, struct mycentroid *points, int order){	
    //initialize the all buckets to 0 
    for (int q = 0; q < Dimensions; q++)
		points[order].values[q] = 0;
    int i,j;
    uchar* p;
    //#pragma omp parallel for
    for( i = 0; i < FRAME_HEIGHT; i++)
    {
    	int ww = i* FRAME_WIDTH;
        for ( j = 0; j < FRAME_WIDTH*3;j = j+3)
        {
			HSV hsv_value = convertBGRtoHSV(image[ww+j], image[ww+j+1], image[ww+j+2]);
			int bucket = Cal_bucket(hsv_value);
			//omp_set_lock(&lock); 
			points[order].values[bucket]++; 
			//omp_unset_lock(&lock); 
        }
    }
	return 0; 
}

int HSV_Histogram(uchar *image, struct mycentroid *points, int order){
	for (int q = 0; q < Dimensions; q++)
		points[order].values[q] = 0;
	int HSV_size = FRAME_WIDTH * FRAME_HEIGHT * sizeof(struct HSV);
	struct HSV* OneImage = (struct HSV *)malloc(HSV_size);
	memcpy(OneImage,image, HSV_size);
	#pragma omp parallel for
	for( int i = 0; i < FRAME_HEIGHT; ++i)
    {
        for ( int j = 0; j < FRAME_WIDTH;j++ )
        {
			int bucket = Cal_bucket(OneImage[i*FRAME_WIDTH+j]);
			omp_set_lock(&lock); 
			points[order].values[bucket]++; 
			omp_unset_lock(&lock); 
        }
    }
    free(OneImage);
    return 0;
}

void test(char* sendline, struct mycentroid *centroids ){
	printf("result\n");
 	int his[QUEUE_LINE][number_centers];
 	memcpy(&his,sendline,sizeof(int) * QUEUE_LINE * number_centers);
 	
 	//test
 	int test_number = 300;
 	string file = "combined.txt";
	ifstream input;
	input.open(file.c_str());
	struct mycentroid *testpoints;
 	testpoints = (struct mycentroid *)malloc(test_number* sizeof(struct mycentroid));
	for (int i = 0; i<test_number;i++){
		for (int j=0; j< Dimensions; j++){
			input >> testpoints[i].values[j];
		}
		testpoints[i].cluster = i/50;  //actual group node
	}
	int count =0;
	for(int ww=0; ww<test_number; ww++)
	{
 		int cluster_number = closet_centroid(testpoints[ww], centroids);
 		int node_number = most_bucket(cluster_number,his);
 		if(node_number == testpoints[ww].cluster)
 			count ++;
 	}
 	printf("correct number: %d over %d \n", count, test_number);
}



double getPSNR_uchar(uchar* current, uchar* previous, int imageSize)
{
	double sse =0.0;
	for(int i =0 ;i < imageSize; i++)
	{
		double diff = double(abs(current[i]- previous[i]));
		sse = sse + diff*diff;
	}
	//printf(" %f\n", sse);
	if( sse <= 1e-10) // for small values return zero
        return 0;
    else
    {
        double mse  = sse / (double)(imageSize);
        double psnr = 10.0 * log10((255 * 255) / mse);
        return psnr;
    }
}


int main(int argc,char *argv[]){
	omp_init_lock(&lock);
	int listenfd = start_connection();
	printf("finished listening\n");
	int LongPower =0;//mW
	int RecentPower[Window];//mW
	/*
	#pragma omp parallel num_threads(2)
	{
		int tid = omp_get_thread_num();
		//power consumption client
		if(tid ==0)
		{
			int CurrentPower = 0;
			char current_power[sizeof(int)];
			int oldest =0;
			int iteration = 0;
			while(1){
				memset(current_power,0,sizeof(int));
				power_recv(current_power, sizeof(int));
				memcpy(&CurrentPower,current_power, sizeof(int));
				LongPower = (LongPower * iteration + CurrentPower)/(iteration + 1);
				RecentPower[oldest]= CurrentPower;
				oldest = (oldest +1)%Window;
				iteration ++;
				printf("LongPower: %d \n", LongPower);
				printf("RecentPower:%d \n", CurrentPower);
				sleep(3);
			}
		}
		
		else
		{
		*/
 	//generate initial centroids and create points structure
 	int centroids_size = sizeof(struct mycentroid)* number_centers;
	int points_size = sizeof(struct mycentroid)* number_points;
	
 	struct mycentroid *centroids;
 	centroids = (struct mycentroid *)malloc(centroids_size);
    
 	struct mycentroid *points;
 	points = (struct mycentroid *)malloc(points_size);
 	
 	initcentroidsParallel(centroids);
 	//initial notification
 	int initial_gate = ser_initialrecv();
 	printf("initial gate: %d \n", initial_gate);
 	
 	int numChanged= 0;
 	int frameNum = 0;
 	int imgSize = FRAME_WIDTH * FRAME_HEIGHT * CHANNEL * sizeof(uchar);
 	int HSV_size = FRAME_WIDTH * FRAME_HEIGHT * sizeof(struct HSV);
 	int psnrTriggerValue = 60;
 	double psnrV;
 	uchar previousFrames[QUEUE_LINE*imgSize];
 	memset(previousFrames,0, QUEUE_LINE*imgSize);
 	struct timeval tpstart,tpend, tpfilter, tpfiltered, tpfiltered_later,tpfeaturesend,assignpoints,assignpointsend,updateend;
	double wholetime, filter_time,features_time, assign_time, global_time;
	wholetime = 0;
	filter_time = 0;
	features_time =0;
	assign_time = 0;
	global_time = 0;
	int num_points =0;
	int loop =0;
 	if(initial_gate == 1)
 	{
 		uchar allframes[QUEUE_LINE*imgSize];
 		memset(allframes,0, QUEUE_LINE*imgSize);
 		while(loop<10){
 			//iteratively receive frames
 			//receive Queue_LINE frames
 			server_recv(allframes, imgSize);
 			if(frameNum % sampling == 0)
 			{
 				///////////////////// frames filters ////////////////////
 				for(int j=0; j< QUEUE_LINE; j++)
 				{
 					gettimeofday(&tpfilter,NULL);
 					psnrV = getPSNR_uchar(&allframes[j*imgSize], &previousFrames[j*imgSize], imgSize);
 					//cout << setiosflags(ios::fixed) << setprecision(3) << psnrV << "dB ";
 					gettimeofday(&tpfiltered,NULL);
 					filter_time = filter_time + 1000000*(tpfiltered.tv_sec-tpfilter.tv_sec)+tpfiltered.tv_usec-tpfilter.tv_usec; //us
 					///////////////////// frames filters ////////////////////
 					
 					if (psnrV < psnrTriggerValue && psnrV)
 					{
 						memcpy(&previousFrames[j*imgSize], &allframes[j*imgSize], imgSize);
 						gettimeofday(&tpfiltered_later,NULL);
        				filter_time = filter_time + 1000000*(tpfiltered_later.tv_sec-tpfiltered.tv_sec)+tpfiltered_later.tv_usec-tpfiltered.tv_usec; //us
        				///////////////////// frames filters ////////////////////
        				
        				
        				///////////////////// histogram features ////////////////////
 						one_image_uchar(&allframes[j*imgSize], points, num_points);
 						gettimeofday(&tpfeaturesend,NULL);
        				features_time = features_time+ 1000000*(tpfeaturesend.tv_sec-tpfiltered_later.tv_sec)+tpfeaturesend.tv_usec-tpfiltered_later.tv_usec; //us      			
 						num_points++;
 						///////////////////// histogram features ////////////////////
 						
 						
 						if(num_points == number_points)
 						{
 							///////////////////// assigned points ////////////////////
 							gettimeofday(&assignpoints,NULL);
 							numChanged = assignPointsToNearestClusterParallel(number_points, points, centroids);
 							gettimeofday(&assignpointsend,NULL);
 							assign_time = assign_time+ 1000000*(assignpointsend.tv_sec-assignpoints.tv_sec)+assignpointsend.tv_usec-assignpoints.tv_usec; //us
 							///////////////////// assigned points ////////////////////
 							
 							///////////////////// global clusters ////////////////////
							UpdateCentroids(number_points, points, centroids);
							gettimeofday(&updateend,NULL);
							global_time = global_time+ 1000000*(updateend.tv_sec-assignpointsend.tv_sec)+updateend.tv_usec-assignpointsend.tv_usec; //us
							///////////////////// global clusters ////////////////////
							
							printf("gate %d, %d points filter used time:%f us\n",initial_gate, number_points, filter_time);
							printf("gate %d, %d points image processing used time:%f us\n",initial_gate, number_points, features_time); 
							printf("gate %d, %d points assigned points used time:%f us\n",initial_gate, number_points, assign_time);
							printf("gate %d, %d points global clusters used time:%f us\n",initial_gate, number_points, global_time);
							if(loop == 1)
							{	
								gettimeofday(&tpstart,NULL);
							}
							else if(loop == 2)
							{	
								gettimeofday(&tpend,NULL);
								wholetime=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec; //us
								printf("gate %d, %d points used time:%f us\n",initial_gate, number_points, wholetime);
							}
							frameNum = -1;
							filter_time = 0;
							features_time =0;
							assign_time = 0;
							global_time = 0;
							num_points =0;
							loop++;
 						}
 					}
 				}
 			}
 			frameNum++;
 		}
 	}
 	else if (initial_gate == 2)
 	{
 		uchar allframes[QUEUE_LINE*imgSize];
 		memset(allframes,0, QUEUE_LINE*imgSize);
 		while(loop<10){
 			//iteratively receive filtered frames
 			//receive Queue_LINE filtered frames
 			server_recv(allframes, imgSize);
 			for(int j=0; j< QUEUE_LINE; j++)
 			{	
 				one_image_uchar(&allframes[j*imgSize], points, num_points);
 				num_points++;
 			}
 			if(num_points  == number_points)
 			{
 				numChanged = assignPointsToNearestClusterParallel(number_points, points, centroids);
				UpdateCentroids(number_points, points, centroids);
				if(loop == 1)
					gettimeofday(&tpstart,NULL);
				else if(loop == 2)
				{	
					gettimeofday(&tpend,NULL);
					wholetime=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec; //us
					printf("gate %d, %d points used time:%f us\n",initial_gate, number_points, wholetime);
				}
				frameNum = -1;
				num_points =0;
				loop++;
 			}
 			frameNum++;
 		}
 	}
 	/*
 	else if (initial_gate == 3)
 	{
		uchar *allframes = (uchar *) malloc(QUEUE_LINE*HSV_size);
 		while(loop<10){
 			//iteratively receive HSV of filtered frames
 			memset(allframes,0,QUEUE_LINE*HSV_size);
 			server_recv(allframes, HSV_size);
 			for(int j=0; j< QUEUE_LINE; j++)
 			{
 				HSV_Histogram(&allframes[j*HSV_size],points,num_points);
 				num_points++ ;
 			}
 			if(num_points == number_points)
 			{
 				numChanged = assignPointsToNearestClusterParallel(number_points, points, centroids);
				UpdateCentroids(number_points, points, centroids);
				if(loop == 1)
					gettimeofday(&tpstart,NULL);
				else if(loop == 2)
				{	
					gettimeofday(&tpend,NULL);
					wholetime=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec; //us
					printf("gate %d, %d points used time:%f us\n",initial_gate, number_points, wholetime);
				}
				frameNum = -1;
				num_points =0;
				loop++;
 			}
 			frameNum++;	
 		}
 		free(allframes);
 	}
 	*/
 	else if (initial_gate == 3)
 	{
 		uchar sendline[QUEUE_LINE*points_size];
 		memset(sendline,0,sizeof(sendline));
 		while(loop<10){
 			//iteratively receive number_points * QUEUE_LINE features
 			server_recv(sendline, points_size);
 			for(int ww=0; ww<QUEUE_LINE; ww++ )
 			{
 				memcpy(points, sendline+ww*points_size, points_size);
 				numChanged = assignPointsToNearestClusterParallel(number_points, points, centroids);
				UpdateCentroids(number_points, points, centroids);
			}
			if(loop == 1)
				gettimeofday(&tpstart,NULL);
			else if(loop == 2)
			{	
				gettimeofday(&tpend,NULL);
				wholetime=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec; //us
				printf("gate %d, %d points used time:%f us\n",initial_gate, number_points*QUEUE_LINE, wholetime);
			}
			loop++;
 		}
 	}
 	else if (initial_gate == 4)
 	{
 		char sendline[centroids_size];
    	memset(sendline,0,centroids_size); 
    	memcpy(sendline, centroids,centroids_size);
    	ser_send(sendline, centroids_size);
    	uchar sendline2[QUEUE_LINE*points_size];
 		memset(sendline2,0,sizeof(sendline2));
 		while(loop<10){
 			//iteratively receive number_points * QUEUE_LINE features
 			server_recv(sendline2, points_size);
 			for(int ww=0; ww<QUEUE_LINE; ww++ )
 			{
 				memcpy(points, sendline2+ww*points_size, points_size);
				UpdateCentroids(number_points, points, centroids);
				
			}
			memset(sendline,0,sizeof(sendline)); 
 			memcpy(sendline, centroids,centroids_size);
 			ser_send(sendline, centroids_size);
			if(loop == 1)
				gettimeofday(&tpstart,NULL);
			else if(loop == 2)
			{	
				gettimeofday(&tpend,NULL);
				wholetime=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec; //us
				printf("gate %d, %d points used time:%f us\n",initial_gate, number_points*QUEUE_LINE, wholetime);
			}
			loop++;
 		}
 	}
 	else if (initial_gate == 5)
 	{	
 		char sendline[centroids_size];
    	memset(sendline,0,centroids_size); 
    	memcpy(sendline, centroids,centroids_size);
    	ser_send(sendline, centroids_size);
 		while(loop<10){
 			//iteratively receive accumulated distances
 			int tag = ser_recv(sendline, centroids, centroids_size);		
 			//if(tag == -1)
 			//	break;
 			//else  //send combined centroids
 			//{
 			memset(sendline,0,sizeof(sendline)); 
 			memcpy(sendline, centroids,centroids_size);
 			ser_send(sendline, centroids_size);
 			//}
			if(loop == 1)
				gettimeofday(&tpstart,NULL);
			else if(loop == 2)
			{	
				gettimeofday(&tpend,NULL);
				wholetime=1000000*(tpend.tv_sec-tpstart.tv_sec)+tpend.tv_usec-tpstart.tv_usec; //us
				printf("gate %d, %d points used time:%f us\n",initial_gate, number_points, wholetime);
			}
			loop++;
 		}
 		//test(sendline, centroids);
 	}
 	else
 	{
 		while(1)
 		{
 			sleep(5);
 		}
 	}
 	free(centroids);
 	free(points);
 	//}
 	//}
 	omp_destroy_lock(&lock);
 	close(listenfd);
 }
 