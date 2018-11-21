#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>
#include <sys/time.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <fcntl.h> // for open
#include<unistd.h> //close()
#include<netinet/in.h>//struct sockaddr_in
#include<arpa/inet.h>//inet_ntoa

#define DEST_PORT 60002
#define DEST_IP_ADDRESS "192.168.0.102"

int connect_server();
char* client_recv(int client_socket, char* recv_buf, int totalsize);
void client_send(int sock_fd, void* sendline, int totalsize);
void close_socket(int sock_fd);