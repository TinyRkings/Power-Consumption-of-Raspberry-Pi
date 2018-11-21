#include "Server_KMeans.h"
#include "Server_socket.h"

cl clientlist[QUEUE_LINE];
cl powerclient;

int start_connection()
{
	int listenfd;
 	struct sockaddr_in addr_serv;
 	listenfd = socket(AF_INET,SOCK_STREAM,0); //create socket
 	if(listenfd < 0){
 		perror("socket");
 		exit(1);
 	}
 	memset(&addr_serv,0,sizeof(addr_serv));
 	addr_serv.sin_family = AF_INET;
 	addr_serv.sin_port = htons(SOURCE_PORT);
 	addr_serv.sin_addr.s_addr = htonl(INADDR_ANY); //automatically get
 	//bind
 	if(bind(listenfd,(struct sockaddr *)&addr_serv,sizeof(struct sockaddr_in))<0){
 		perror("bind");
 		exit(1);
 	}
 	//listen
 	//if (listen(listenfd,QUEUE_LINE+1) < 0){
 	if (listen(listenfd,QUEUE_LINE) < 0){
 		perror("listen");
 		 exit(1);
 	}
 	printf("======waiting for client's request======\n");
 	ser_accept(listenfd);
 	return listenfd;
}

//server accepts the connection from client
void ser_accept(int listenfd)
{
  struct sockaddr_in remote_addr;
  socklen_t sin_size = sizeof(struct sockaddr_in);
  int connect_fd = -1;  
  int iteration =0;
  int first =0;
  while(iteration < QUEUE_LINE){
  //while(iteration < QUEUE_LINE){
    if ((connect_fd = accept(listenfd, (struct sockaddr *)&remote_addr, &sin_size)) == -1)
    {
      printf("accept socket error: %s(errno: %d)", strerror(errno), errno);
      continue;
    }
    /*
    if(first == 0){
    	powerclient.port = remote_addr.sin_port;
    	powerclient.ip = inet_ntoa(remote_addr.sin_addr);
    	powerclient.c_fd = connect_fd;
    	printf("powerclient %s:%d:\n", powerclient.ip, powerclient.port);
    	first++;
    }
    */
    //iteration ++;
    
   // else{
    
    	clientlist[iteration].port = remote_addr.sin_port;
    	clientlist[iteration].ip = inet_ntoa(remote_addr.sin_addr);
    	clientlist[iteration].c_fd = connect_fd;
    	printf("client %s:%d:\n", clientlist[iteration].ip, clientlist[iteration].port);
    	iteration++;
   // }
    
  }
}

//initial notifications from all pis
int ser_initialrecv(){
	int tag[QUEUE_LINE];
	int int_size = sizeof(int);
	#pragma omp parallel num_threads(QUEUE_LINE)
	{
		char buff[int_size];
		memset(buff,0,int_size);
		int tid = omp_get_thread_num();
		//char* starting =buff;
		char* moving = buff;
		int tagsize = int_size;
		while (tagsize>0){
			int RecvSize= recv(clientlist[tid].c_fd, moving, tagsize, 0);
			tagsize = tagsize - RecvSize;
			moving += RecvSize;
		}
		memcpy(&tag[QUEUE_LINE-1],buff, int_size);	
	}
	return tag[0];
}
//server receives power consumption
void power_recv(char* currentpower, int size){
	int totalsize = size;
	char* moving = currentpower;
	while (totalsize>0){
		int RecvSize= recv(powerclient.c_fd, moving, totalsize, 0);
		totalsize = totalsize - RecvSize;
		moving += RecvSize;
	}
}

//server receives data from all clients
void server_recv(uchar* allimages, int size){
	#pragma omp parallel num_threads(QUEUE_LINE)
	{
		int tid = omp_get_thread_num();
		int totalsize = size;
		//uchar* starting =&allimages[tid*size];
		uchar* moving = &allimages[tid*size];
		while (totalsize>0){
			int RecvSize= recv(clientlist[tid].c_fd, moving, totalsize, 0);
			totalsize = totalsize - RecvSize;
			moving += RecvSize;
		}
		//memcpy(&allimages[tid*size],starting,size);
  	}
}

//server receives data from all clients
int ser_recv(char* sendline, struct mycentroid *centroids, int centroids_size){
	struct mycentroid allcentroids[QUEUE_LINE][number_centers];
	int allweights[QUEUE_LINE];
	memset(&allcentroids,0,QUEUE_LINE*centroids_size);
	memset(&allweights,0,sizeof(int)*QUEUE_LINE);
	int his[QUEUE_LINE][number_centers];
	#pragma omp parallel num_threads(QUEUE_LINE)
	{
		char buff[QUEUE_LINE*centroids_size+sizeof(int)];
		memset(buff,0,sizeof(buff));
		int tid = omp_get_thread_num();
		char* moving = buff;
		int tagsize = sizeof(int);
		while (tagsize>0){
			int RecvSize= recv(clientlist[tid].c_fd, moving, tagsize, 0);
			tagsize = tagsize - RecvSize;
			moving += RecvSize;
		}		
		int tag;
		memcpy(&tag,buff, sizeof(int));	
		
		if(tag == -1){  //histogram	
			int totalsize = sizeof(int) * number_centers;
			while (totalsize>0){
				int RecvSize= recv(clientlist[tid].c_fd, moving, totalsize, 0);
				totalsize = totalsize - RecvSize;
				moving += RecvSize;
			}
			memcpy(&his[tid],buff + sizeof(int),sizeof(int) * number_centers);
			allweights[tid] =-1;
		}
		else{	
			int totalsize = centroids_size;
			while (totalsize>0){
				int RecvSize= recv(clientlist[tid].c_fd, moving, totalsize, 0);
				totalsize = totalsize - RecvSize;
				moving += RecvSize;
			}
  			allweights[tid] = tag;
			memcpy(&allcentroids[tid],buff + sizeof(int),centroids_size);
		}
  	}
  	
	if(allweights[0]==-1){ //histogram
			memcpy(sendline, his,sizeof(int)* QUEUE_LINE* number_centers);//convert struct into char array
			return -1;
	}
	//total weight
	int sumweight[number_centers];
	for (int w=0 ;w< number_centers; w++){
		sumweight[w] = 0;
		for (int q =0; q< QUEUE_LINE; q++)
			sumweight[w] = sumweight[w] + allcentroids[q][w].cluster;
	}
	
	#pragma omp parallel for
	for(int i=0; i< number_centers; i++){
		for (int j=0; j< Dimensions; j++){
			centroids[i].values[j] = 0;
			for (int m=0; m< QUEUE_LINE; m++){
				//centroids[i].values[j] += allcentroids[m][i].values[j] * allcentroids[m][i].cluster;
				centroids[i].values[j] += allcentroids[m][i].values[j];
			}
			if(sumweight[i] != 0)
				centroids[i].values[j] = centroids[i].values[j] / sumweight[i];
		}
	} 
    return 0;
}


//send to all clients
void ser_send(char* sendline, int mm_size){
	char * send_array = (char*) malloc(mm_size*QUEUE_LINE);
	for(int i=0; i<QUEUE_LINE; i++)
	{
		memcpy(send_array+ i*mm_size,sendline, mm_size);	
	}
	#pragma omp parallel num_threads(QUEUE_LINE)
	{
		int tid = omp_get_thread_num();
		int totalsize = sizeof(struct mycentroid)* number_centers;
		char* each_start = send_array+tid*mm_size;
		while (totalsize >0){
        	int SendSize= send(clientlist[tid].c_fd, each_start, totalsize, 0);
        	totalsize = totalsize - SendSize;
        	each_start += SendSize;
        }
	}
}