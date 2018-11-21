#include "Client.h"
#include "ParallelKMeans.h"


//connect to server 
int connect_server(){
	int sock_fd;
    struct sockaddr_in addr_serv; //server address
     
    sock_fd = socket(AF_INET,SOCK_STREAM,0); //create socket
    if(sock_fd < 0){
    	printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
    	exit(1);
    }
	memset(&addr_serv,0,sizeof(addr_serv));
    addr_serv.sin_family = AF_INET;
    addr_serv.sin_port =  htons(DEST_PORT);
    addr_serv.sin_addr.s_addr = inet_addr(DEST_IP_ADDRESS);
     
    //connect to server
    if(connect(sock_fd,(struct sockaddr *)&addr_serv,sizeof(struct sockaddr)) < 0){
    	printf("connect error: %s(errno: %d)\n", strerror(errno), errno);
     	exit(1);
    }
    return sock_fd;   
}

//receive data from server
char* client_recv(int client_socket, char* recv_buf, int totalsize){
  //accept data from server
  char * starting = recv_buf;
  while (totalsize>0){
  	int RecvSize= recv(client_socket, recv_buf, totalsize, 0);
  	totalsize = totalsize - RecvSize;
  	recv_buf += RecvSize;
  }
  return starting;
  
}

//send data to server
void client_send(int sock_fd, void* sendline, int totalsize){
	int SendSize = 0;
	uchar* moving = (uchar*)sendline;
	while (totalsize >0){
    	SendSize= send(sock_fd, moving, totalsize, 0);
        totalsize = totalsize - SendSize;
        moving += SendSize;
    }
}

 
void close_socket(int sock_fd){
	close(sock_fd);
}
 