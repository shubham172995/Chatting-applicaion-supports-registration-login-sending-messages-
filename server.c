//I COULD HAVE CREATED AN ARRAY TO CHECK USERNAMES AND PASSWORDS IF THEY MATCH BUT WON'T WORK SINCE REGISTRATION AND LOGIN ARE HAPPENING IN DIFFERENT PROCESSES.
#include<stdio.h>
#include<stdlib.h>
#include<stdbool.h>
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

#define BACKLOG 10

#define MAXDATASIZE 200

int people=0;
char loggedin[100][100];

void sigchld_handler(int s){
	int saved_errno=errno;

	while(waitpid(-1, NULL, WNOHANG)>0);

	errno=saved_errno;
}

void *get_in_addr(struct sockaddr *sa){
	if(sa->sa_family==AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]){
	/*if(argc!=3){
		fprintf(stderr, "usage:- Username Password\n");
		return 1;
	}*/
	int sockfd_register, sockfd_login, new_fd_register, new_fd_login;
	struct addrinfo hints, *servinfo_register, *servinfo_login, *p;
	struct sockaddr_storage their_addr_login, their_addr_register;
	socklen_t sin_size;
	struct sigaction sa;
	int numbytes=0;
	int yes=1;
	char s[INET6_ADDRSTRLEN], buf[MAXDATASIZE];
	int rv_register, rv_login;

	memset(&hints, 0, sizeof hints);
	hints.ai_family=AF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_flags=AI_PASSIVE;

	if((rv_register=getaddrinfo(NULL, REG_PORT, &hints, &servinfo_register))!=0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_register));
		return 1;
	}

	for(p=servinfo_register; p!=NULL; p=p->ai_next){
		if((sockfd_register=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
			perror("server:socket");
			continue;
		}

		if(setsockopt(sockfd_register, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1){
			perror("setsockopt\n");
			exit(1);
		}

		if(bind(sockfd_register, p->ai_addr, p->ai_addrlen)==-1){
			close(sockfd_register);
			perror("server:bind\n");
			continue;
		}

		break;
	}

	if(p==NULL){
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if((rv_login=getaddrinfo(NULL, LOGIN_PORT, &hints, &servinfo_login))!=0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_login));
		return 1;
	}

	for(p=servinfo_login; p!=NULL; p=p->ai_next){
		if((sockfd_login=socket(p->ai_family, p->ai_socktype, p->ai_protocol))==-1){
			perror("server:socket");
			continue;
		}

		if(setsockopt(sockfd_login, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))==-1){
			perror("setsockopt\n");
			exit(1);
		}

		if(bind(sockfd_login, p->ai_addr, p->ai_addrlen)==-1){
			close(sockfd_login);
			perror("server:bind\n");
			continue;
		}

		break;
	}

	if(p==NULL){
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	freeaddrinfo(servinfo_login);
	freeaddrinfo(servinfo_register);

	if(listen(sockfd_register, BACKLOG)==-1){
		perror("listen\n");
		exit(1);
	}

	if(listen(sockfd_login, BACKLOG)==-1){
		perror("listen\n");
		exit(1);
	}

	sa.sa_handler=sigchld_handler;	//REAP ALL DEAD PROCESSES.
	sigemptyset(&sa.sa_handler);
	sa.sa_flags=SA_RESTART;
	if(sigaction(SIGCHLD, &sa, NULL)==-1){
		perror("sigaction\n");
		exit(1);
	}

	//signal(SIGCHLD, SIG_IGN);	//SO THAT PARENT DOESN'T HAVE TO WAIT FOR CHILD TO EXIT.

	printf("server waiting for connections\n");

	FILE *fp;

	if(!fork()){	//for child process, connect to login port.
		while(1){
			sin_size=sizeof their_addr_login;
			new_fd_login=accept(sockfd_login, (struct sockaddr*)&their_addr_login, &sin_size);

			if(new_fd_login==-1){
				perror("accept\n");
				continue;
			}

			inet_ntop(their_addr_login.ss_family, get_in_addr((struct sockaddr*)&their_addr_login), s, sizeof s);
			printf("Server got connection from %s\n", s);

			if(!fork()){	//For child process, receive.
				close(sockfd_register);	//Child will not need any listeners.
				close(sockfd_login);
				if((numbytes=recv(new_fd_login, buf, MAXDATASIZE-1, 0))==-1){
					perror("recv\n");
					close(new_fd_login);
					exit(1);
				}
				int ulength=0, plength=0;
				bool check=0;
				int i=0;
				while(buf[i]){
					if(buf[i]==' '){
						check=1;
					}
					if(buf[i]!=' '&&!check)
						++ulength;
					if(buf[i]!=' '&&check){
						++plength;
					}
					++i;
				}
				char user[ulength], pass[plength];
				int in=0;i=0;
				check=0;
				while(buf[i]){
					if(buf[i]==' '){
						check=1;
					}
					if(buf[i]!=' '&&!check)
						user[i]=buf[i];
					if(buf[i]!=' '&&check){
						pass[in]=buf[i];
						++in;
					}
					++i;
				}
				/*printf("%s %d %s %d\n", user, strlen(user), pass, strlen(pass));
				printf("%d\n", people);*/
				bool flag=0;
				fp=fopen("./database.txt", "r");
				if(fp){
					int lenline=0, read=0;
					char ch, *line;
					//printf("File reading successful\n");
					while((read=getline(&line, &lenline, fp))!=-1){
						/*if(ch!='\n' || (ch=='\n' && i<lenline)){
							memset(line, 0, sizeof line);
							continue;
						}*/
						char temp[lenline];
						for(i=0;i<ulength;i++)
							temp[i]=user[i];
						temp[ulength]=' ';
						for(i=0;i<plength;i++)
							temp[ulength+1+i]=pass[i];
						temp[ulength+plength+1]='\n';
						//printf("%sa\n", line);
						//printf("%sa", temp);
						if(strcmp(line, temp)==0){
							flag=1;
							break;
						}
					}
					printf("OUT\n");
				}

				if(!flag){
					if((numbytes=send(new_fd_login, "Login unsuccessful\n", 19, 0))==-1){
						perror("send\n");
						close(new_fd_register);
						exit(1);
					}
				}
				else{
					if((numbytes=send(new_fd_login, "Login Successful! Hurray!\n", 27, 0))==-1){
						perror("send\n");
						close(new_fd_login);
						exit(1);
					}
					else{
						for(int i=0;i<ulength;i++)
							loggedin[people][i]=user[i];
						++people;
					}
				}

				printf("%d\n", people);

				memset(buf, 0, sizeof buf);
				if((numbytes=recv(new_fd_login, buf, MAXDATASIZE-1, 0))==-1){
					perror("recv\n");
					exit(1);
				}

				printf("Message from client: %s\n", buf);

				close(new_fd_register);
				exit(0);
			}
			close(new_fd_register);
		}	
	}

	else{	//for Parent, connect to register port.
		while(1){
			sin_size=sizeof their_addr_register;
			new_fd_register=accept(sockfd_register, (struct sockaddr*)&their_addr_register, &sin_size);

			if(new_fd_register==-1){
				perror("accept\n");
				continue;
			}

			inet_ntop(their_addr_register.ss_family, get_in_addr((struct sockaddr*)&their_addr_register), s, sizeof s);
			printf("Server got connection from %s\n", s);

			if(!fork()){	//For child process, receive.
				close(sockfd_register);	//Child will not need any listeners.
				close(sockfd_login);
				if((numbytes=recv(new_fd_register, buf, MAXDATASIZE-1, 0))==-1){
					perror("recv\n");
					close(new_fd_register);
					exit(1);
				}
				fp=fopen("./database.txt", "a");
				if(fp){
					//putc('\n', fp);
					int i=0, in=0;
					bool check=0;
					while(buf[i]){
						putc(buf[i], fp);
						/*if(buf[i]==' '){
							check=1;
							usernames[people][i]='\0';	//THIS WON'T WORK SINCE I AM MAKING CHANGES TO USERNAMES ARRAY IN THIS PARTICULAR PROCESS. IT WON'T REFLECT IN LOGIN PROCESS.
						}
						if(buf[i]!=' '&&!check)
							usernames[people][i]=buf[i];
						if(buf[i]!=' '&&check){
							passwords[people][in]=buf[i];
							++in;
						}*/
						++i;
					}
					putc('\n', fp);
				}

				fclose(fp);	//CLOSING SO THAT IF BEFORE THIS PROCESS EXITS, SOME PROCESS TRIES TO LOG IN WITH THIS USERNAME AND PASSWORD, SHOULD BE ABLE TO DO IT.

				if((numbytes=send(new_fd_register, "Registration Successful\n", 24, 0))==-1){
					perror("send\n");
					close(new_fd_register);
					exit(1);
				}

				memset(buf, 0, sizeof buf);
				if((numbytes=recv(new_fd_register, buf, MAXDATASIZE-1, 0))==-1){
					perror("recv\n");
					exit(1);
				}

				printf("Message from client: %s\n", buf);

				close(new_fd_register);
				exit(0);
			}
			close(new_fd_register);
		}
	}
	/*fp=fopen("./database.txt", "r");
	char *line;
	if(!fp){
		fprintf(stderr, "File not found\n");
		return 1;
	}
	bool connectflag=0;
	while((read=getline(&line, fp))!=-1){
		char *a=argv[1], *b=line;
		int len=0;
		int passlength=0;
		while(a++==b++){
			++len;
		}
		if(*(line+len)==' '){
			b=(line+len+1);
			a=argv[2];
			while(a++==b++){
				++passlength;
			}
			if(*(line+len+1+passlength)!='\n'){
				fprintf(stderr, "Incorrect Password\n");
				return 1;
			}
			else connectflag=1;
			break;
		}
	}
	if(connectflag==0){
		fclose(fp);
		fp=fopen("./database.txt", "a");
	}*/
}