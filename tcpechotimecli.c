/* Copyright (c) 2015 All Right Reserved,Arpit Singh(arpsingh@cs.stonybrook.edu)
 * tcpechotomecli.c file contains client code, it can request two services
 * 1. Echo 2.Time. Client create new process for each request. See README for more details
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
#include <netinet/in.h>
#include "unp.h"
#define port_echo 62005
#define port_time 62006
//Handle child on SIGNAL
void child_handler(int sign){
        while(waitpid(-1,NULL,WNOHANG) > 0);
}
//Signal handler
void signal_handler(int sign){
	printf(" \n signal generated to close parent or child \n");
	exit(0);

}
//Function to parse address/hostname passed
void *get_host(char *host, char *addr){
	struct hostent *host_detail = NULL;
	struct in_addr host_ip;

	int ret = inet_pton(AF_INET,host,&host_ip);
	if (ret > 0){
		host_detail = gethostbyaddr(&host_ip,sizeof(host_ip),AF_INET);
	} else {
		host_detail = gethostbyname(host);
	}
	if (host_detail == NULL){
		return NULL;
	} else {
		if (inet_ntop(AF_INET,host_detail->h_addr,addr,INET_ADDRSTRLEN) > 0) {
			printf("Connecting to server %s with ip address %s",host_detail->h_name,addr);
			return host_detail;
		} else {
			return NULL;
		}
	}
}
int main(int argc,char *argv[])
{
	struct hostent *host_detail;
        char addr[INET_ADDRSTRLEN] = "";
	
        
	if (argc != 2){
                printf(" \n Pass right no of Parameters \n");
                exit(1);

        }
	host_detail = (struct hostent *) get_host(argv[1],addr);
	if (host_detail == NULL) {
		printf("\n invalid server name \n");
		exit(1);
	}
	signal(SIGCHLD,child_handler);
	signal(SIGPIPE,SIG_IGN);
	//signal(SIGTERM,signal_handler);
	//signal(SIGINT,signal_handler);
	
	for(;;) {
		int option;
		int pid;
		int pipe_fd[2];
		while (1) {
        		printf(" \n Enter \n 1 for Echo \n 2 for time \n 3 for quit \n");
        		if (scanf("%d",&option) != 1){
				char temp[4096];
				fgets(temp,4096,stdin);

			}
			if (option == 1 || option == 2){
				break;

			} 
			if (option == 3){
				printf(" \n Quitting Client \n");
				return 0;
			}
			printf(" \n wrong option selected \n");
		}
		if (pipe(pipe_fd) == -1) {
			err_sys("\n pipe error");
		}
		pid = fork();
	
		if (pid < 0){
			err_sys("error in fork");

		} else if (pid == 0){
			//execution of child process
			//close read pipe in child
			close(pipe_fd[0]);
			char snp_fd[10];
			snprintf(snp_fd,10,"%d",pipe_fd[1]);
		
			if (option == 1){
				execlp("xterm","xterm","-e","./echo_cli",addr,snp_fd,(char *) 0);
			} else if (option == 2){
				execlp("xterm","xterm","-e","./time_cli",addr,snp_fd,(char *) 0);
			} else {

				printf("\n invalid option \n");
			}

		} else { 
			//close write pipe in parent
			close(pipe_fd[1]);	

			int max_fd = pipe_fd[0] + 1;
			fd_set read_set;
			FD_ZERO(&read_set);
			
			while (1){
				char rcvbuf[4096];
				int msg_len;
				//Select procedure to detect read/write on sockets and stdin
				FD_SET(fileno(stdin),&read_set);
				FD_SET(pipe_fd[0],&read_set);
				int sel = select(max_fd,&read_set,NULL,NULL,NULL);
				if (sel < 0) {
					printf("\n Server Down client cannot connect \n");
				}
				//To find out if input is being passed to parent
				if (FD_ISSET(fileno(stdin),&read_set)) {
					memset(rcvbuf,0,4096);
					fgets(rcvbuf,4096,stdin);
					printf(" \n Please write in child process %s \n",rcvbuf);
				}
				//reading the message sent by child
				if (FD_ISSET(pipe_fd[0],&read_set)) {
					msg_len = read(pipe_fd[0],rcvbuf,4096);
					if (msg_len > 0) {
						rcvbuf[msg_len] = '\0';
						if (fputs(rcvbuf,stdout) == EOF){
							printf("\n error in priting \n");
						}
						break;

					}
					break;
				}

	             }	
		     close(pipe_fd[0]);
		}
	}

        return 0;
}
                                      
