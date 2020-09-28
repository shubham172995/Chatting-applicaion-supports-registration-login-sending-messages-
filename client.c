#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<sys/wait.h>
#include<signal.h>

#define REG_PORT "3490"
#define LOGIN_PORT "3591"

#define MAXDATASIZE 200

void* get_in_addr(struct sockaddr *sa){
	if(sa->sa_family==AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]){
	if(argc!=4){
		fprintf(stderr, "Usage:- Program_Name \"Login/Register\" Username Password\n");
	}
	int sockfd, numbytes;
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family=AF_UNSPEC;	//IF YOU WANT IPv4, do AF_INET
	hints.ai_socktype=SOCK_STREAM;

	printf("hey %s %s\n", argv[1], argv[2]);

	if(strcmp(argv[1], "Login")==0){
		//Connect to Login Port
		if((rv=getaddrinfo("localhost", LOGIN_PORT, &hints, &servinfo))!=0){
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}

		for(p=servinfo;p!=NULL;p=p->ai_next){
			if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
				perror("client:socket\n");
				continue;
			}
			if(connect(sockfd, p->ai_addr, p->ai_addrlen)==-1){
				close(sockfd);
				perror("client: connect\n");
				continue;
			}
			break;
		}

		if(p==NULL){
			fprintf(stderr, "client: failed to connect\n");
			return 2;
		}

		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
		printf("client: connecting to %s\n", s);

		freeaddrinfo(servinfo);

		int length=strlen(argv[2])+strlen(argv[3]);
		if(length+1>MAXDATASIZE){
			fprintf(stderr, "Arguements too big...\n");
			return 2;
		}
		for(int i=0;i<strlen(argv[2]);i++)
			buf[i]=argv[2][i];
		buf[strlen(argv[2])]=' ';
		for(int i=0;i<strlen(argv[3]);i++)
			buf[i+strlen(argv[2])+1]=argv[3][i];
		buf[length+1]='\0';

		if((numbytes=send(sockfd, buf, length+2, 0))==-1){
			perror("send\n");
			exit(1);
		}

		printf("client sent:- %s\n\n", buf);
		memset(buf, 0, sizeof buf);

		if((numbytes=recv(sockfd, buf, MAXDATASIZE, 0))==-1){
			perror("recv\n");
			exit(1);
		}

		printf("Message from server: %s\n", buf);

		if(strcmp(buf, "Login unsuccessful\n")==0){
			close(sockfd);
			return 0;
		}

		printf("Enter \"exit\" to exit \n Enter \"who\" to get who all are logged in \n Enter \"msg\" followed by name of user to send message\n");
		printf("Enter \"create_grp\" followed by usernames and \'group name\' to create a group\n");
		printf("Enter \"join_grp\" followed by \'group name\' to join group\n");
		printf("Enter \"send\" followed by username to send a file\n");
		printf("Enter \"msg_group\" followed by \'group name\' to broadcast message to group\n");
		printf("Enter \"recv\" to receive file from someone\n");

		char str[12];
		scanf("%s", str);

		//printf("%s\n", str);

		if(strcmp("exit", str)==0){
			close(sockfd);
		}

		else{
			if((numbytes=send(sockfd, str, 12, 0))==-1){
				perror("send\n");
				exit(1);
			}
		}

		return 0;
	}
	
	else if(strcmp(argv[1], "Register")==0){
		//Connect to Registration Port
		if((rv=getaddrinfo("localhost", REG_PORT, &hints, &servinfo))!=0){
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			return 1;
		}

		for(p=servinfo;p!=NULL;p=p->ai_next){
			if((sockfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
				perror("client:socket\n");
				continue;
			}
			if(connect(sockfd, p->ai_addr, p->ai_addrlen)==-1){
				close(sockfd);
				perror("client: connect\n");
				continue;
			}
			break;
		}

		if(p==NULL){
			fprintf(stderr, "client: failed to connect\n");
			return 2;
		}

		inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
		printf("client: connecting to %s\n", s);

		freeaddrinfo(servinfo);

		int length=strlen(argv[2])+strlen(argv[3]);
		char message[length+2];
		for(int i=0;i<strlen(argv[2]);i++)
			message[i]=argv[2][i];
		message[strlen(argv[2])]=' ';
		for(int i=0;i<strlen(argv[3]);i++)
			message[i+strlen(argv[2])+1]=argv[3][i];
		message[length+1]='\n';
		printf("%d %d %d %s\n", strlen(argv[2]), strlen(argv[3]), length, message);

		if((numbytes=send(sockfd, message, length+2, 0))==-1){
			perror("send\n");
			exit(1);
		}

		printf("client sent:- %s\n", message);

		memset(&message, 0, sizeof message);

		if((numbytes=recv(sockfd, message, MAXDATASIZE, 0))==-1){
			perror("receive\n");
			exit(1);
		}

		printf("server sent:- %s\n", message);

		printf("Enter \"exit\" to exit \n Enter \"who\" to get who all are logged in \n Enter \"msg\" followed by name of user to send message\n");
		printf("Enter \"create_grp\" followed by usernames and \'group name\' to create a group\n");
		printf("Enter \"join_grp\" followed by \'group name\' to join group\n");
		printf("Enter \"send\" followed by username to send a file\n");
		printf("Enter \"msg_group\" followed by \'group name\' to broadcast message to group\n");
		printf("Enter \"recv\" to receive file from someone\n");

		char str[12];
		scanf("%s", str);

		//printf("%s\n", str);

		if(strcmp("exit", str)==0){
			close(sockfd);
		}

		else{
			if((numbytes=send(sockfd, str, 12, 0))==-1){
				perror("send\n");
				exit(1);
			}
		}

		return 0;
	}

	else{
		fprintf(stderr, "Invalid Usage\n");
		return 1;
	}
}