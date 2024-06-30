#include<stdio.h>
#include<unistd.h>
#include<linux/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<arpa/inet.h>
#include<syslog.h>
#include<errno.h>
#include<signal.h>
#include<stdbool.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>

#define PORT "9000"
#define BACKLOG 5
#define BUFSIZE 1000
#define OUTPUT "/var/tmp/aesdsocketdata"

bool terminate = false;
static void signal_handler(int signal_number){
	if((signal_number == SIGTERM) || (signal_number == SIGINT )){
		syslog(LOG_USER, "Caught signal, exiting\n");
		//printf("Caught signal, exiting\n");
		//stop the connection
		terminate = true;
	}
}

struct package{
	char* package;
	int packindex;
	int packsize;
};

struct package* init_package(){
	struct package* p = (struct package*)malloc(sizeof(struct package));
	p -> packindex = 0;
	p -> packsize = BUFSIZE;
	p -> package = (char*)malloc(sizeof(char)*(p -> packsize));
	return p;
}

void extend_package(struct package* p){
	p -> packsize += BUFSIZE;
	p -> package = (char*)realloc(p -> package, sizeof(char)*(p -> packsize));
}

void free_package(struct package* p){
	free(p -> package);
	free(p);
}

int accept_data(int sockfd, const char* path, struct package* pack){

	struct sockaddr ser_addr;
	memset(&ser_addr, 0, sizeof(struct sockaddr));
	socklen_t ser_addr_len = 0;
	int fd = accept(sockfd, &ser_addr, &ser_addr_len);
	if(fd < 0){
		printf("Error %d (%s) for accept", errno, strerror(errno));
		return -1;
	}

	if(terminate){
		return 0;
	}
	
	char* ip = NULL;
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);

	ip = inet_ntoa(((struct sockaddr_in *)&ser_addr) -> sin_addr);	
	//printf("Accepted connection from %s\n", ip);
	syslog(LOG_USER, "Accepted connection from %s", ip);

	int rt = 0;
	while((rt = recv(fd, buf, BUFSIZE, 0)) > 0){
		//printf("%s", buf);
		//printf("%d bytes are received\n", rt);
		
		int f = open(path, O_CREAT | O_APPEND | O_WRONLY, 0664);
		if(f == -1){
			printf("Error %d (%s) of openning file", errno, strerror(errno));
		}
		write(f, buf, rt);
		//printf("%d bytes are written into the file\n", ss);
		close(f);
		while(rt > pack -> packsize - pack -> packindex){
			extend_package(pack);
		}
		strncpy(&pack->package[pack -> packindex], buf, rt);
		pack -> packindex += rt;
		
		//resend the package
		if(rt < BUFSIZE){
			//printf("packindex %d\n", pack -> packindex);
			send(fd, pack -> package, pack -> packindex, 0);
			//printf("%d bytes are send back to server\n", ss);
		}
	}
	close(fd);
	//printf("Close connection from %s\n", ip);
	syslog(LOG_USER, "Close connection from %s", ip);
	return 0;
}

int main(int argc, char* argv[]){
	
	bool deamon = false;
	int ch;
	while((ch = getopt(argc, argv, "d")) != -1){
		if(ch == 'd'){
			deamon = true;
		}
	}
	openlog(NULL, 0, LOG_USER);

	//IPv4 configuration
	struct addrinfo hints;
	struct addrinfo* servinfo;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if(getaddrinfo(NULL, PORT, &hints, &servinfo) != 0){
		printf("Error %d (%s)", errno, strerror(errno));
		return -1;
	}

	int sockfd = socket(servinfo -> ai_family,\
			servinfo -> ai_socktype,\
			servinfo -> ai_protocol);
	if(sockfd == -1){
		printf("Error %d (%s)", errno, strerror(errno));
		return -1;
	}
	if(bind(sockfd, servinfo -> ai_addr, servinfo -> ai_addrlen) != 0){
		printf("Error %d (%s)", errno, strerror(errno));
		return -1;
	}
	if(listen(sockfd, BACKLOG) != 0){
		printf("Error %d (%s)", errno, strerror(errno));
		return -1;
	}

	
	struct package* pack = init_package();
	if(deamon){
		pid_t pid = fork();
		if(!pid){
			//registering signal
			struct sigaction sig;
			memset(&sig, 0, sizeof(struct sigaction));
			sig.sa_handler=signal_handler;
			if(sigaction(SIGINT, &sig, NULL) != 0){
				printf("Error %d (%s) registering for SIGINT\n", errno, strerror(errno));
				return -1;
			}
			if(sigaction(SIGTERM, &sig, NULL) != 0){
				printf("Error %d (%s) registering for SIGTERM\n", errno, strerror(errno));
				return -1;
			}
			//signal registering complete

			while(1){
				accept_data(sockfd, OUTPUT, pack);
				if(terminate){
					break;
				}			
			}
		}
	}
	else{
		//registering signal
		struct sigaction sig;
		memset(&sig, 0, sizeof(struct sigaction));
		sig.sa_handler=signal_handler;
		if(sigaction(SIGINT, &sig, NULL) != 0){
			printf("Error %d (%s) registering for SIGINT\n", errno, strerror(errno));
			return -1;
		}
		if(sigaction(SIGTERM, &sig, NULL) != 0){
			printf("Error %d (%s) registering for SIGTERM\n", errno, strerror(errno));
			return -1;
		}
		//signal registering complete
	
		while(1){
			accept_data(sockfd, OUTPUT, pack);
			if(terminate){
				break;
			}
		}
	}
	//close connection
	close(sockfd);

	//free memory
	free_package(pack);
	freeaddrinfo(servinfo);

	//remove buffer file
	remove(OUTPUT);
	
	return 0;
}
