/* Copyright (c) 2015 All Right Reserved,Arpit Singh(arpsingh@cs.stonybrook.edu)
 * tcpechotomesrv.c file contains server code, it creates two services
 * 1. Echo 2.Time. Server is multithreaded. See README for more details
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include "unp.h"
#define backlog 10
static int sock_flags;
static int flag = 0;
//function to handle signals
void signal_handler(int sign){

	if (sign == SIGINT)
		err_sys("\n Interrupt receievd \n");

	if (sign == SIGPIPE)
		err_sys("\n pipe error \n");
	
	flag = 1;
	exit(0);

}

//Thread to execute echo code at server
static void *server_echo(void *args) {
       int fd;
       fd = *((int *)args);
	//printf(" \n Received fd is %d \n",fd);
       char buffer[4096];
       int msg_len;
	
	if (pthread_detach(pthread_self()) != 0){
		close(fd);
		err_sys("pthread error");
	}
try:
	while((msg_len = Readline(fd,buffer,4096)) > 0)
	{       //printf("message receievd %s  %d \n",buffer,msg_len);
		if (write(fd,buffer,msg_len) != msg_len)
			printf(" \n error in writing \n");
	}
	if (msg_len < 0  && errno ==  EINTR) {
		goto try;
	} else if (msg_len < 0) {
		printf(" \n error in reading \n");
	} else if (msg_len == 0)
		printf("\n Client closed the echo service connection \n");

	close(fd);
	return NULL;

}
//Thread to execute time code at server
static void *server_time(void *args){
        int fd,max_fd;
	time_t ticks;
	struct timeval tv;
	fd_set read_set;
	fd = *((int *)args);
	//printf(" \n fd received is %d \n",fd);
        char buffer[4096];
        int msg_len;
	int sel_ret;
	int bytes;
         if (pthread_detach(pthread_self()) != 0){
                err_sys("pthread error");
         }
	 max_fd = fd + 1;
	FD_ZERO(&read_set);
	tv.tv_sec =0;
	tv.tv_usec = 0;
	signal(SIGPIPE, SIG_IGN);
	while(1)
	{
		FD_SET(fd,&read_set);
		sel_ret = select(fd,&read_set,NULL,NULL,&tv);
		if (sel_ret < 0 ) {
			printf(" \n select error \n");
			break;
		}
		if (sel_ret > 0 ) {

			bytes = Readline(fd,buffer,4096);
			if (bytes == 0 || errno == EINTR || errno == EPIPE){
				printf("\n FIN received \n");
				break;

			}
			if (bytes > 0){
			
				printf(" \n Client sending garbage data \n");	
			} else {
				printf("\n Client Closing Connection \n");
				close(fd);
				break;
			}
			if ( bytes  == 0 || errno == EINTR || errno == EPIPE ){
				printf("\n Client terminated \n");
				close(fd);
				return NULL;

			}

		} else if (sel_ret == 0) {
			if (errno == EINTR){
				close(fd);
				return NULL;	

			}	
			ticks = time(NULL);
			snprintf(buffer,4096,"%.24s\r\n",ctime(&ticks));
		        msg_len = strlen(buffer);
			//printf("sending time %s",buffer);
			if (flag == 1 || errno == EPIPE){
				Close(fd);
			        return NULL;	

			}
			if (write(fd,buffer,msg_len) != msg_len){
				printf("\n Client closed the time service connectionn \n");
				break;
				printf(" \n write error \n");

			}
		}
		tv.tv_sec = 5;
		tv.tv_usec = 0;
	}

	close(fd);
	return NULL;
}	
//Initializing services of echo and client
int initialize_server(char *listen_port) {
        struct addrinfo criteria,*info_server,*local_info;
        int addr_ret;
        int listen_fd,conn_fd;
        struct sockaddr conn_addr;
        struct sigaction signal_action;
        socklen_t sin_size = sizeof(conn_addr);
        memset(&criteria,0,sizeof(criteria));
        criteria.ai_family = AF_UNSPEC;
        criteria.ai_socktype = SOCK_STREAM;
        criteria.ai_flags = AI_PASSIVE;
        addr_ret = getaddrinfo(NULL,listen_port,&criteria,&info_server);
        if (addr_ret != 0){
                err_sys("add_ret is %d  in error\n",addr_ret);
        }

        int yes = 1;
        for (local_info = info_server;local_info != NULL;local_info = local_info->ai_next){
                listen_fd = socket(local_info->ai_family,local_info->ai_socktype,local_info->ai_protocol);
                if (listen_fd == -1){
                	printf("\n error in socket command");
                	continue;
                }

                //printf("\n listen fd is %d \n",listen_fd);
                int setopt = setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
                if (setopt == -1){
                        printf("setsockoption error %d",setopt);
                        exit(1);

                }
                int b = bind(listen_fd,(SA *) info_server->ai_addr,info_server->ai_addrlen);
                if (b == -1){
                        close(listen_fd);
                        printf(" \n bind error");
                        continue;
                }
                //printf("\n bind successful %d \n",b);
                break;
        }
        freeaddrinfo(info_server);

        if (local_info == NULL){
                err_sys("server failed to bind");

        }
	sock_flags = fcntl(listen_fd,F_GETFL,0);
	if (sock_flags == -1){
		err_sys("Error in socket flags by fcntl");
	}
	int non_b = fcntl(listen_fd,F_SETFL,sock_flags | O_NONBLOCK);
	if (non_b == -1) {
		err_sys(" \n fcntl cannot set non bloacking \n");
	}
        int l = listen(listen_fd,10);
	//printf("\n listen succesful %d \n",l);
        if (l == -1){
                err_sys("listen unsuccesful %d \n",l);

        }
	return listen_fd;


}
//On client request create thread for each request
static void create_thread(int listen_fd,void * (*func)(void *)){
	struct sockaddr_in conn_addr;
	pthread_t tid;
	socklen_t sin_size = sizeof(conn_addr);
	//printf("\n creating new thread listen fd is %d \n",listen_fd);
        int conn_fd = accept(listen_fd,(SA *) &conn_addr,&sin_size);
        if (conn_fd == -1){
              err_sys("error in accept \n"); 
        }
	if (fcntl(conn_fd,F_SETFL,sock_flags)  == -1) {	
		printf("fcntl failed to reset socket options error %s",strerror(errno));
		exit(EXIT_FAILURE);
	}
	char addr_buf[100];
	inet_ntop(AF_INET,&conn_addr.sin_addr,addr_buf,sizeof(addr_buf));
	printf("\n New connection, ip address : %s ,port %d \n",addr_buf,conn_addr.sin_port);
	int *fd  = malloc(sizeof(int));
	*fd = conn_fd;
	pthread_create(&tid,NULL,func,(void *)fd);
	return;
}

int main(int argc,char *argv[]){
	int fd_echo,fd_time,max_fd;
	fd_set r_fd;

	if (argc != 1){

		printf(" \n wrong arguments passed \n");
		exit(0);
	}
	char port_echo[6];
	char port_time[6];
	strcpy(port_echo,"62005");
	strcpy(port_time,"62006");
	fd_echo = initialize_server(port_echo);
	fd_time = initialize_server(port_time);
        printf("\n Echo and Time service initialized, waiting for connections \n");	
	if (fd_echo > fd_time){
		max_fd = fd_echo +1;
	} else {
		max_fd = fd_time +1;
	}
	signal(SIGPIPE, SIG_IGN);
	signal(SIGINT,signal_handler);
	signal(SIGTERM,signal_handler);
	FD_ZERO(&r_fd);
	//printf("\n returned fd for echo and listen are %d and %d and max_fd is %d \n",fd_echo,fd_time,max_fd);
	for(;;){
		FD_SET(fd_echo,&r_fd);
		FD_SET(fd_time,&r_fd);

		if (select(max_fd,&r_fd,NULL,NULL,NULL) < 0) {
			err_sys(" \n Error in calling select \n");
		}
		if (FD_ISSET(fd_echo,&r_fd)) {
			printf("\n server received echo request %d\n",fd_echo);
			create_thread(fd_echo,server_echo);
		}
		if (FD_ISSET(fd_time,&r_fd)) {
			printf("\n server received time request fd is %d \n ",fd_time);
			create_thread(fd_time,server_time);
		}
	}
	return 1;
}
