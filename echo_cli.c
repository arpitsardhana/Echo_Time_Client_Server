/* Copyright (c) 2015 All Right Reserved,Arpit Singh(arpsingh@cs.stonybrook.edu)
 * echo_cli.c file contains echo client code ,Echo client executes in xterm
 *  Sends data typed in STDIN to server.See README for more details
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netdb.h>
#include <signal.h>
#include "unp.h"
#define port "62005"
static int parentfd = -1;

//get ip address to connect
void *get_in_addr(struct sockaddr *sa){
        if (sa->sa_family == AF_INET){
                return &(((struct sockaddr_in*)sa)->sin_addr);

        } else {
                return NULL;
        }

}
// function to send messages to parent
static void send_msg_to_parent(char *msg,int exit_status) {
	char buffer[4096] ;
	strcpy(buffer,"Client Echo: Message :");
	strcat(buffer,msg);
	int msg_len = strlen(buffer);
	write(parentfd,buffer,msg_len);	
	exit(exit_status);
}

//function to handle signals
static int signal_handler(int signal){
	if (signal == SIGPIPE){
		send_msg_to_parent("client terminated, server crashed",EXIT_SUCCESS);
	} else 
		send_msg_to_parent("client terminated, no error",EXIT_SUCCESS);

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
        memset(&criteria,0,sizeof(criteria));
        criteria.ai_family = AF_UNSPEC;
        criteria.ai_socktype = SOCK_STREAM;
	criteria.ai_flags = AI_PASSIVE;
	int i = 0;
      char newbuf[4096];
       printf("Enter data to be echoed ,to exit press cntrl+c \n");

        if (argc != 3){
                send_msg_to_parent("pass right parameters",EXIT_FAILURE);

        }
	parentfd = atoi(argv[2]);
        addr_ret = getaddrinfo(argv[1],port,&criteria,&info_server);
        if (addr_ret != 0){
                send_msg_to_parent("getaddrinfo failed with error\n",EXIT_FAILURE);
        }

        for (local_info = info_server;local_info != NULL;local_info = local_info->ai_next){
                fd = socket(local_info->ai_family,local_info->ai_socktype,local_info->ai_protocol);
                if (fd == -1){
			printf("socket command failed with error\n");
                        continue;
                }

                int b = connect(fd,(SA *)info_server->ai_addr,info_server->ai_addrlen);
                if (b == -1){
                        close(fd);
                        printf(" \n connect error %d and %s \n",b,strerror(errno));
                        continue;
                }
                break;
        }

        freeaddrinfo(info_server);

        if (local_info == NULL){
                send_msg_to_parent("client failed to connect",EXIT_FAILURE);
        }
	if (signal(SIGINT,(Sigfunc *)signal_handler) == SIG_ERR){
		send_msg_to_parent("signal error",EXIT_FAILURE);

	}
	signal(SIGTERM,(Sigfunc *)signal_handler);
	signal(SIGPIPE,(Sigfunc *)signal_handler);
	int max_fd = fd + 1;
	FD_ZERO(&read_set);
	for(;;){
		//Select to monitor STDIN and input received from server
		FD_SET(fileno(stdin),&read_set);
		FD_SET(fd,&read_set);
		int sel_ret = select(max_fd,&read_set,NULL,NULL,NULL);
		if (sel_ret < 0) {
			send_msg_to_parent("select error",EXIT_FAILURE);

		}
		if (FD_ISSET(fileno(stdin),&read_set)){
			char new_buf[4096];
			if (fgets(sendbuf,4096,stdin) != NULL) {
				int msg_len = strlen(sendbuf);
				//write back to server written on STDIN
				if (write(fd,sendbuf,msg_len) != msg_len){
					send_msg_to_parent("write error",EXIT_FAILURE);
				}

			} else {
				//Cntrl+C or Cntrl+d passed on client
				send_msg_to_parent("Client terminated",EXIT_SUCCESS);
			}
		}
		//on output received from server
		if (FD_ISSET(fd,&read_set)) {
			if ( readline(fd,rcvbuf,4096) > 0 ) {
				printf("Server Message :");
				if (fputs(rcvbuf,stdout) == EOF) {
					send_msg_to_parent("fputs and output error",EXIT_FAILURE);
				}
			} else {
				//Server sends FIN/RST to close connection
				send_msg_to_parent("sever not responding",EXIT_FAILURE);

			}
		}

	}
	return 0;
}

