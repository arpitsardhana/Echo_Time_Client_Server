/* Copyright (c) 2015 All Right Reserved,Arpit Singh(arpsingh@cs.stonybrook.edu)
 * time_cli.c file contains client time code,t requests current daytime from server 
 * Server sends back current daytime. See README for more details
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include "unp.h"
#define port "62006" 
static int parentfd = -1;
//Get the address of passed ip address
void *get_in_addr(struct sockaddr *sa){
        if (sa->sa_family == AF_INET){
                return &(((struct sockaddr_in*)sa)->sin_addr);
        } else {
                return NULL;
        }

}
//sending message to parent
static void send_msg_to_parent(char *msg,int exit_status) {
        char buffer[4096] ;
        strcpy(buffer,"Client Echo: Message :");
        strcat(buffer,msg);
	strcat(buffer," \n");
        int msg_len = strlen(buffer);
        write(parentfd,buffer,msg_len);
        exit(exit_status);
}
//function to handle SIGTERM and SIGINT
static int signal_handler(int signal){
        send_msg_to_parent("client terminated, no error \n",EXIT_SUCCESS);
}
int main(int argc,char *argv[])
{

        struct addrinfo criteria,*info_server,*local_info;
        int addr_ret;
        int fd,conn_fd;
        char buf[100];
        char rcvbuf[4096];
        char sendbuf[4096];
        fd_set read_set;
	char temp_buf[4096];
        memset(&criteria,0,sizeof(criteria));
        criteria.ai_family = AF_UNSPEC;
        criteria.ai_socktype = SOCK_STREAM;
        criteria.ai_flags = AI_PASSIVE;
        if (argc != 3){
                send_msg_to_parent("pass right parameters",EXIT_FAILURE);
                exit(1);

        }
        parentfd = atoi(argv[2]);
        addr_ret = getaddrinfo(argv[1],port,&criteria,&info_server);
        if (addr_ret != 0){
		send_msg_to_parent("getaddrinfo failed",EXIT_FAILURE);
        }

        for (local_info = info_server;local_info != NULL;local_info = local_info->ai_next){
                fd = socket(local_info->ai_family,local_info->ai_socktype,local_info->ai_protocol);
                if (fd == -1){
                        printf("\n error in socket command");
                        continue;
                }

                int b = connect(fd,(SA *)info_server->ai_addr,info_server->ai_addrlen);
                if (b == -1){
                        close(fd);
                        printf(" \n connect error");
                        continue;
                }
                break;
        }

        freeaddrinfo(info_server);

        if (local_info == NULL){
                printf("client failed to connect");
                exit(1);

        }
        if (signal(SIGINT,(Sigfunc *)signal_handler) == SIG_ERR)  {
                send_msg_to_parent("signal error",EXIT_FAILURE);
	}
	signal(SIGTERM,(Sigfunc *)signal_handler);

	 int max_fd = fd + 1;
        FD_ZERO(&read_set);
        for(;;){
		//Select to monitor STDIN and output from Server
                FD_SET(fileno(stdin),&read_set);
                FD_SET(fd,&read_set);
		//Since stdin is 1, thus max_fd = fd+1
               int sel_ret = select(max_fd,&read_set,NULL,NULL,NULL);
                if (sel_ret < 0) {
                        send_msg_to_parent("select error",EXIT_FAILURE);

                }
                if (FD_ISSET(fileno(stdin),&read_set)){
                        char new_buf[4096];
			memset(sendbuf,0,4096);
                        if (fgets(sendbuf,4096,stdin) != NULL) {
				 if (sendbuf == NULL || *sendbuf == EOF || sendbuf ==0){

					send_msg_to_parent("Terminated with cntrl+d",EXIT_SUCCESS); 
				}  
                                int msg_len = strlen(sendbuf);
				if (msg_len == 0)  {
					printf("cntrl+d hit \n");
				}
			        printf(" \n Please dont write in time client window \n");	
                        } else {
				//printf("cntrl d hit fgets null");
 				send_msg_to_parent("Client terminated by cntrl+d",EXIT_SUCCESS);
                        }
                }
		//When output received from server
                if (FD_ISSET(fd,&read_set)) {
                        if ( Readline(fd,rcvbuf,4096) > 0 ) {
				printf("Server Message :");
                                if (fputs(rcvbuf,stdout) == EOF) {
                                        send_msg_to_parent("fputs and output error",EXIT_FAILURE);
                                }
                        } else {
                                send_msg_to_parent("sever not responding",EXIT_FAILURE);

                        }
                }

        }
        return 0; 

}
