#include <time.h>
#include "ParallelKMeans.h"
#include "Client.h"

using namespace std;
using namespace cv;

void test_histogram(int sock_fd, struct mycentroid * points, int number_points){
	//time variables
    clock_t startTime=0, endTime=0;
    double parallelTime=0;
    
    int int_size= sizeof(int);
	char sendline[int_size+ int_size* number_centers];
	//histogram
	int his[number_centers];
	
	startTime = clock();
	compute_histogram(number_points,points,his);
	int iteration_end = -1;
	memset(sendline,0,sizeof(sendline));    
    memcpy(sendline, &iteration_end, int_size);
   	memcpy(sendline + int_size, his,int_size* number_centers);//convert struct into char array
	client_send(sock_fd, sendline, int_size+ int_size* number_centers);
	
	endTime=clock();
	parallelTime = ((double) (endTime - startTime)) * 1000 / CLOCKS_PER_SEC; //ms
	cout << "Final time: " << parallelTime << endl;
	
	cout << "Final histogram: " << endl;
	for(int i = 0; i< number_centers; i++)
		cout << i<<":	"<<his[i]<<endl;
}

double getPSNR(const Mat& I1, const Mat& I2)
{
    Mat s1;
    absdiff(I1, I2, s1);       // |I1 - I2|
    s1.convertTo(s1, CV_32F);  // cannot make a square on 8 bits
    s1 = s1.mul(s1);           // |I1 - I2|^2

    Scalar s = sum(s1);        // sum elements per channel

    double sse = s.val[0] + s.val[1] + s.val[2]; // sum channels

    if( sse <= 1e-10) // for small values return zero
        return 0;
    else
    {
        double mse  = sse / (double)(I1.channels() * I1.total());
        double psnr = 10.0 * log10((255 * 255) / mse);
        return psnr;
    }
}


int one_image(Mat& image, struct mycentroid *points, int order){
	if(! image.data )	// Check for invalid input
    {
        cout <<  "Could not open or find the image" << std::endl ;
        return -1;
    }
	int channels = image.channels();
    int nRows = image.rows;
    int nCols = image.cols * channels;
    if (image.isContinuous())
    {
        nCols *= nRows;
        nRows = 1;
    }
    //initialize the all buckets to 0 
    for (int q = 0; q < Dimensions; q++)
		points[order].values[q] = 0;
    int i,j;
    uchar* p;
    //#pragma omp parallel for
    for( i = 0; i < nRows; i++)
    {
       	p = image.ptr<uchar>(i);
        for ( j = 0; j < nCols; )
        {
			HSV hsv_value = convertBGRtoHSV(p[j], p[j+1], p[j+2]);
			int bucket = Cal_bucket(hsv_value);
			j = j+3;
			//omp_set_lock(&lock); 
			points[order].values[bucket]++;
			//omp_unset_lock(&lock); 
            //p[j] = table[p[j]];
        }
    }
	return 0; 
}


int image2HSV(Mat& image, struct HSV* OneImage){
	if(! image.data )	// Check for invalid input
    {
        cout <<  "Could not open or find the image" << std::endl ;
        return -1;
    }
	int channels = image.channels();
    int nRows = image.rows;
    int nCols = image.cols * channels;
    int Cols = image.cols;
    if (image.isContinuous())
    {
        nCols *= nRows;
        nRows = 1;
    }
    int i,j;
    uchar* p;
    #pragma omp parallel for
    for( i = 0; i < nRows; ++i)
    {
       	p = image.ptr<uchar>(i);
        for ( j = 0; j < nCols; )
        {
			HSV hsv_value = convertBGRtoHSV(p[j], p[j+1], p[j+2]);
			OneImage[i*Cols+j/3].H = hsv_value.H;
			OneImage[i*Cols+j/3].S = hsv_value.S;
			OneImage[i*Cols+j/3].V = hsv_value.V;
			j = j+3;
        }
    }
	return 0; 
}

int main(){
	omp_init_lock(&lock);
	/*
	////////////////////power measurement//////////////////////////
	// Create I2C bus
	int file;
	char *bus = "/dev/i2c-1";
	if ((file = open(bus, O_RDWR)) < 0) 
	{
		printf("Failed to open the bus. \n");
		exit(1);
	}
	// Get I2C device, ADS1115 I2C address is 0x48(72)
	ioctl(file, I2C_SLAVE, 0x48);
	int generate_power, filter_power, features_power, assign_power, accumulate_power, wifi_power;
	generate_power = 0;
	filter_power= 0;
	features_power = 0;
	assign_power = 0;
	accumulate_power = 0;
	wifi_power = 0;
	int CurrentPower=0;
	////////////////////power measurement//////////////////////////
	*/
    int numChanged;
    srand(time(NULL));
    
    //parameters of data
	int number_points = 180;
	int int_size = sizeof(int);
	int centroids_size = sizeof(struct mycentroid)* number_centers;
	int points_size = sizeof(struct mycentroid)* number_points;
	
	//create the points space and K centers space
	struct mycentroid *centroids;
 	centroids = (struct mycentroid *)malloc(centroids_size);
 	
 	struct mycentroid *points;
 	points = (struct mycentroid *)malloc(points_size);
 	
	//connect to server
	int sock_fd = connect_server();
	cout<<"********successfully connected************"<<endl;
	
	//video capture
    int psnrTriggerValue = 60;
    VideoCapture captRefrnc(0);
    if (!captRefrnc.isOpened())
    {
        cout  << "Could not open reference "  << endl;
        return -1;
    }
    Size refS = Size((int) captRefrnc.get(CAP_PROP_FRAME_WIDTH),
                     (int) captRefrnc.get(CAP_PROP_FRAME_HEIGHT));
    int Fps = (int) captRefrnc.get(CAP_PROP_FPS);
    
    // Windows
    //const char* WIN_RF = "Reference";
    //namedWindow(WIN_RF, WINDOW_AUTOSIZE);
    //moveWindow(WIN_RF, 400, 0);       

    cout << "Reference frame resolution: Width=" << refS.width << "  Height=" << refS.height
         << " of Fps :# " << captRefrnc.get(CAP_PROP_FPS) << endl;
    int gate = 1;
    int sampling =30;
    
    //initial notification
    char sendline1[int_size];
    memset(sendline1,0,int_size); 
    memcpy(sendline1, &gate, int_size);
    client_send(sock_fd, (void*)sendline1, int_size);
    
    if (gate == 5 || gate == 4)
    {
    	//receive initial centroids
    	char sendline[centroids_size];
		char* moving = sendline;
		char* recv;
		memset(sendline,0,centroids_size); 
		recv = client_recv(sock_fd, sendline, centroids_size);
		memcpy(centroids, recv, centroids_size);
		//printf("received initial centroids \n");
    }
	
    Mat frameReference;
    Mat LastEffective = Mat::zeros(480,640, CV_8UC3);
    struct timeval tpgenerate, tpgenerateend, tpfilter, tpfiltered, tpfiltered_later,tpfeaturesend,assignpoints,assignpointsend,distanceend;
	double generate_time, filter_time,features_time, assign_time, accumulate_time;
	double psnrV;
	
	int collected = 0;
    generate_time = 0;
	filter_time = 0;
	features_time=0;
	assign_time=0;
	accumulate_time =0;
	int generate_iteration =0;
	int filter_iteration = 0;
	int features_iteration =0;
	int assign_iteration = 0;
	int accumulate_iteration = 0;
	int wifi_iteration = 0;
	char sendline[centroids_size+int_size];
	char* moving = sendline;
	memset(sendline,0,sizeof(sendline));
	char sendline2[points_size];
    char* moving2 = sendline2;
    memset(sendline2,0,sizeof(sendline2));
    char* recv;	
    for(;;) //Show the image captured in the window and repeat
    {
	//printf("gate %d, %d points wifi power:%d \n",gate, number_points, wifi_power);
		printf("gate %d, %d points generating frames used time:%f us\n",gate, number_points, generate_time); 
    	for(int frameNum=0; frameNum < number_points * sampling; frameNum++)
    	{
    		///////////////////// generate frames ////////////////////
    		gettimeofday(&tpgenerate,NULL);
       		captRefrnc >> frameReference;
       		int imgSize = frameReference.total()*frameReference.elemSize();
        	if (frameReference.empty())
        	{
            	break;
        	}
        	gettimeofday(&tpgenerateend,NULL);
        	generate_time = generate_time + 1000000*(tpgenerateend.tv_sec-tpgenerate.tv_sec)+tpgenerateend.tv_usec-tpgenerate.tv_usec; //us
        	///////////////////// generate frames ////////////////////
        	/*
        	CurrentPower = Current_Power(file);
        	generate_power = (generate_power * generate_iteration + CurrentPower)/(generate_iteration + 1);
        	generate_iteration ++;
        	*/
        	if(gate == 1) //add a placement
        	{
        		//Send Frames
        		client_send(sock_fd, (void *)frameReference.data, imgSize);
        		/*
        		CurrentPower = Current_Power(file);
        		wifi_power = (wifi_power * wifi_iteration + CurrentPower)/(wifi_iteration + 1);
        		wifi_iteration ++;
        		*/
        	}
        	else if (gate == 2) //frames filter: sampling and distinct
        	{	
        		if(frameNum%sampling == 0)
        		{	
        			psnrV = getPSNR(frameReference,LastEffective);// lower, more different
        			//cout << setiosflags(ios::fixed) << setprecision(3) << psnrV << "dB";
        			if (psnrV < psnrTriggerValue && psnrV)
        			{	
        				LastEffective = frameReference.clone();
        				client_send(sock_fd, (void *)frameReference.data, imgSize);
        			}
        			/*
        			memcpy(moving, frameReference.data, imgSize);//convert struct into char array
    				moving = moving + imgSize;
    				collected++ ;
    				if(collected == number_points)
    					break;
    				*/
        		}        		
        	}
        	/*
        	else if (gate == 3)   //frames filter and RGV2HSV 
        	{
        		if(frameNum%sampling == 0)
        		{	
        			psnrV = getPSNR(frameReference,LastEffective);// lower, more different
        			cout << setiosflags(ios::fixed) << setprecision(3) << psnrV << "dB";
        			if (psnrV < psnrTriggerValue && psnrV)
        			{	
        				LastEffective = frameReference.clone();
        				struct HSV* OneImage = (struct HSV *)malloc(frameReference.total()* sizeof(struct HSV));
        				image2HSV(frameReference, OneImage);
        				client_send(sock_fd, (void*)OneImage, frameReference.total()* sizeof(struct HSV));
						free(OneImage);
        			}
    			}
        	}
        	*/
        	else if (gate == 3) //HSV histogram
        	{
        		if(frameNum%sampling == 0)
        		{	
        			psnrV = getPSNR(frameReference,LastEffective);// lower, more different
        			//cout << setiosflags(ios::fixed) << setprecision(3) << psnrV << "dB";
        			if (psnrV < psnrTriggerValue && psnrV)
        			{	
        				LastEffective = frameReference.clone();
        				one_image(frameReference, points, collected);
        				collected++ ;
        				if(collected == number_points)
    					{
    						memcpy(moving2, points, points_size);//convert struct into char array
    						client_send(sock_fd, (void*)sendline2, points_size);
    						break;
    					}
        			}
        			
    			}
        	}
        	else if (gate == 4) // assigned points
        	{
        		if(frameNum%sampling == 0)
        		{	
        			psnrV = getPSNR(frameReference,LastEffective);// lower, more different
        			if (psnrV < psnrTriggerValue && psnrV)
        			{	
        				LastEffective = frameReference.clone();
        				one_image(frameReference, points, collected);
        				collected++ ;
        				if(collected == number_points)
    					{
    						numChanged = assignPointsToNearestClusterParallel(number_points, points, centroids);
    						memcpy(moving2, points, points_size);//convert struct into char array
    						client_send(sock_fd, (void*)sendline2, points_size);
    						
    						//receive new centroids from the master
							memset(sendline,0,sizeof(sendline));
							recv = client_recv(sock_fd, sendline, centroids_size);
							memcpy(centroids, recv, centroids_size);
    						break;
    					}
        			}
        			
    			}
        	}
        	else  if (gate == 5)//frames filter, vector features and accumulated distances
        	{
        		if(frameNum%sampling == 0)
        		{
        			///////////////////// frames filters ////////////////////
        			gettimeofday(&tpfilter,NULL);
        			psnrV = getPSNR(frameReference,LastEffective);// lower, more different
        			//cout << setiosflags(ios::fixed) << setprecision(2) << psnrV << "dB ";
        			gettimeofday(&tpfiltered,NULL);
        			filter_time = filter_time + 1000000*(tpfiltered.tv_sec-tpfilter.tv_sec)+tpfiltered.tv_usec-tpfilter.tv_usec; //us
        			///////////////////// frames filters ////////////////////
        			
        			if (psnrV < psnrTriggerValue && psnrV)
        			{	
        				LastEffective = frameReference.clone();
        				gettimeofday(&tpfiltered_later,NULL);
        				filter_time = filter_time + 1000000*(tpfiltered_later.tv_sec-tpfiltered.tv_sec)+tpfiltered_later.tv_usec-tpfiltered.tv_usec; //us
        				///////////////////// frames filters ////////////////////
        				/*
        				CurrentPower = Current_Power(file);
        				filter_power = (filter_power * filter_iteration + CurrentPower)/(filter_iteration + 1);
        				filter_iteration ++;
        				*/
        				///////////////////// histogram features ////////////////////
        				one_image(frameReference, points, collected);
        				gettimeofday(&tpfeaturesend,NULL);
        				features_time = features_time+ 1000000*(tpfeaturesend.tv_sec-tpfiltered_later.tv_sec)+tpfeaturesend.tv_usec-tpfiltered_later.tv_usec; //us
        				collected++ ;
        				///////////////////// histogram features ////////////////////
        				/*
        				CurrentPower = Current_Power(file);
        				features_power = (features_power * features_iteration + CurrentPower)/(features_iteration + 1);
        				features_iteration ++;
        				*/
        				if(collected == number_points)
						{
							///////////////////// assigned points ////////////////////
							gettimeofday(&assignpoints,NULL);
        					numChanged = assignPointsToNearestClusterParallel(number_points, points, centroids);
        					gettimeofday(&assignpointsend,NULL);
        					assign_time = assign_time+ 1000000*(assignpointsend.tv_sec-assignpoints.tv_sec)+assignpointsend.tv_usec-assignpoints.tv_usec; //us
        					///////////////////// assigned points ////////////////////
        					/*
        					CurrentPower = Current_Power(file);
        					assign_power = (assign_power * assign_iteration + CurrentPower)/(assign_iteration + 1);
        					assign_iteration ++;
        					*/
        					///////////////////// local clusters ////////////////////
							accumulatedDistanceParallel(number_points, points, centroids);
							gettimeofday(&distanceend,NULL);
							accumulate_time = accumulate_time + 1000000*(distanceend.tv_sec-assignpointsend.tv_sec)+distanceend.tv_usec-assignpointsend.tv_usec; //us
							///////////////////// local clusters ////////////////////
							/*
							CurrentPower = Current_Power(file);
        					accumulate_power = (accumulate_power * accumulate_iteration + CurrentPower)/(accumulate_iteration + 1);
        					accumulate_iteration ++;
							*/
							printf("gate %d, %d points generating frames used time:%f us\n",gate, number_points, generate_time); 
							printf("gate %d, %d points filter used time:%f us\n",gate, number_points, filter_time);
							printf("gate %d, %d points image processing used time:%f us\n",gate, number_points, features_time); 
							printf("gate %d, %d points assigned points used time:%f us\n",gate, number_points, assign_time);
							printf("gate %d, %d points local clusters used time:%f us\n",gate, number_points, accumulate_time);
							/*
							printf("gate %d, %d points generating power:%d \n",gate, number_points, generate_power); 
							printf("gate %d, %d points filter power:%d \n",gate, number_points, filter_power);
							printf("gate %d, %d points image processing power:%d \n",gate, number_points, features_power); 
							printf("gate %d, %d points assigned points power:%d \n",gate, number_points, assign_power);
							printf("gate %d, %d points local clusters power:%d \n",gate, number_points, accumulate_power);
							*/
							//send new centroids and number of points to the master
    						memcpy(moving, &number_points, sizeof(int));
    						moving = moving + sizeof(int);
    						memcpy(moving, centroids, centroids_size);//convert struct into char array
							client_send(sock_fd, (void*)sendline, centroids_size+ sizeof(int));
			
							//receive new centroids from the master
							memset(sendline,0,sizeof(sendline));
							recv = client_recv(sock_fd, sendline, centroids_size);
							memcpy(centroids, recv, centroids_size);
							collected = 0;
    						generate_time = 0;
							filter_time = 0;
							features_time=0;
							assign_time=0;
							accumulate_time =0;
							/*
							generate_power = 0;
							filter_power= 0;
							features_power = 0;
							assign_power = 0;
							accumulate_power = 0;
							wifi_power = 0;
							*/
							break;
						}
        				
        			}
        			
    				
        		}
        	}
        	//imshow(WIN_RF, frameReference);}
        	//char c = (char)waitKey(1000/Fps);
        	//if (c == 27) break;
        }
    }
	test_histogram(sock_fd, points, number_points);
	
	free(centroids);
	free(points);
	//close_socket(sock_fd);
	omp_destroy_lock(&lock);
    return 0;
}
