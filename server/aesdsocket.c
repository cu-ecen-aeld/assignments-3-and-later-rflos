#include<stdio.h>
#include <time.h>
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
#include<sys/queue.h>
#include<pthread.h>

#define PORT "9000"
#define BACKLOG 5
#define BUFSIZE 1000
#define OUTPUT "/var/tmp/aesdsocketdata"

bool terminate = false;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

static void signal_handler(int signal_number){
	if((signal_number == SIGTERM) || (signal_number == SIGINT )){
		syslog(LOG_USER, "Caught signal, exiting\n");
		printf("Caught signal, exiting\n");
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

void custom_timer(union sigval argv){ 
        char outstr[200];
        char buffer[250];
        time_t t;
        struct tm *tmp;
        struct package* p = (struct package*)argv.sival_ptr;
        
        int s = pthread_mutex_lock(&mtx);
        if(s != 0){
                printf("Error %d (%s) of mutex lock", errno, strerror(errno));
        }
        t = time(NULL);
        tmp = localtime(&t);
        strftime(outstr, sizeof(outstr), "%a, %d %b %Y %T %z", tmp);
        int l = sprintf(buffer, "timestamp:%s\n", outstr);
        while(l > p -> packsize - p -> packindex){
                extend_package(p);
        }
        strncpy(&p-> package[p -> packindex], buffer, l);
        p -> packindex += l;
        int f = open(OUTPUT, O_CREAT | O_APPEND | O_WRONLY, 0664);
        if(f == -1){
                printf("Error %d (%s) of openning file", errno, strerror(errno));
        }
        write(f, buffer, l);
        close(f);
        s = pthread_mutex_unlock(&mtx);
        if(s != 0){
                printf("Error %d (%s) of mutex unlock", errno, strerror(errno));
        }
}

struct rec{
        pthread_t t;            //thread for receiving
	int complete;           //complete flag
	int fd;                 //file description for accepting package
	struct package* pack;   //package buffer for receiving data
        char ip[100];
	SLIST_ENTRY(rec) recs;
};

void* rec_thread(void* argv){
	struct rec* rev = (struct rec*)argv;
	char buf[BUFSIZE];
	memset(buf, 0, BUFSIZE);
        

	syslog(LOG_USER, "Accepted connection from %s", rev -> ip);
        
	int rt = 0;
        int s = pthread_mutex_lock(&mtx);
        if(s != 0){
                printf("Error %d(%s) of mutex lock\n", errno, strerror(errno));
        }
	while((rt = recv(rev->fd, buf, BUFSIZE, 0)) > 0){
                //write to file
                int f = open(OUTPUT, O_CREAT | O_APPEND | O_WRONLY, 0664);
                if(f == -1){
                        printf("Error %d (%s) of openning file", errno, strerror(errno));
                }
                write(f, buf, rt);
                close(f);

                //extend package if requires and copy the buf to the package
                while(rt > rev -> pack -> packsize - rev -> pack -> packindex){
			extend_package(rev -> pack);
		}
		strncpy(&rev -> pack-> package[rev -> pack -> packindex], buf, rt);
		rev -> pack -> packindex += rt;
			
		//send the package back to host
		if(rt < BUFSIZE){
			send(rev -> fd, rev -> pack -> package, rev -> pack -> packindex, 0);
		}
	}
        s = pthread_mutex_unlock(&mtx);
        if(s != 0){
                printf("Error %d (%s) of mutex unlock", errno, strerror(errno));
        }
        
        syslog(LOG_USER, "Close connection from %s", rev -> ip);
	rev -> complete = 1;
	return NULL;
}

SLIST_HEAD(slisthead, rec);

int connection_handler(int sockfd, struct package* p){

	struct slisthead head;
        struct rec* n;
        SLIST_INIT(&head);
        
	struct sockaddr ser_addr;
	memset(&ser_addr, 0, sizeof(struct sockaddr));
	socklen_t ser_addr_len = 0;
	int fd = accept(sockfd, &ser_addr, &ser_addr_len);
	if(fd < 0){
		printf("Error %d (%s) for accept\n", errno, strerror(errno));
	}
	if(terminate){
                while(!SLIST_EMPTY(&head)){
                        n = SLIST_FIRST(&head);
                        pthread_join(n->t, NULL);
                        SLIST_REMOVE_HEAD(&head, recs);
                        free(n);
                }
                return 0;
	}

        char* ip = NULL;
	char ipbuf[BUFSIZE];
	memset(ipbuf, 0, BUFSIZE);
	ip = inet_ntoa(((struct sockaddr_in *)&ser_addr) -> sin_addr);	
	
        //create thread in the list
        n = (struct rec*)malloc(sizeof(struct rec));
        n -> complete = 0;
        n -> fd = fd;
        n -> pack = p;
        //copy ip to the structure
        for(int i = 0; ip[i] != '\0'; i++){
                n -> ip[i] = ip[i];
        }
        int t = pthread_create(&(n->t), NULL, rec_thread, (void*)n);
        if(t != 0){
                printf("Error %d (%s) for thread creating",errno, strerror(errno));
        }
        SLIST_INSERT_HEAD(&head, n, recs);
        SLIST_FOREACH(n, &head, recs)
                if(n->complete == 1){
                        pthread_join(n->t, NULL);
                        SLIST_REMOVE(&head, n, rec, recs);
                        free(n);
                }
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

	
	if(deamon){ 
		if(!daemon(0, 0)){
                        struct package* pack = init_package();
			
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
                        
                        timer_t timerid;
                        struct sigevent evp;
                        struct itimerspec it;

                        memset(&evp, 0, sizeof(struct sigevent));
                        evp.sigev_value.sival_ptr = (void*)pack;
                        evp.sigev_notify = SIGEV_THREAD;
                        evp.sigev_notify_function = custom_timer;
                        
                        if(timer_create(CLOCK_REALTIME, &evp, &timerid) == -1){
                                printf("Error %d (%s) creating timer\n", errno, strerror(errno));
                                return -1;
                        }

                        it.it_interval.tv_sec = 10;
                        it.it_interval.tv_nsec = 0;
                        it.it_value.tv_sec = 1;
                        it.it_value.tv_nsec = 0;
                        
                        if(timer_settime(timerid, 0, &it, NULL) == -1){
                                printf("Error %d (%s) setting timer\n", errno, strerror(errno));
                                return -1;

                        }
			while(1){
                                connection_handler(sockfd, pack);
				if(terminate){
					break;
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
	}
	else{
	        struct package* pack = init_package();
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
                
                timer_t timerid;
                struct sigevent evp;
                struct itimerspec it;

                memset(&evp, 0, sizeof(struct sigevent));
                evp.sigev_value.sival_ptr = (void*)pack;
                evp.sigev_notify = SIGEV_THREAD;
                evp.sigev_notify_function = custom_timer;
                
                if(timer_create(CLOCK_REALTIME, &evp, &timerid) == -1){
                        printf("Error %d (%s) creating timer\n", errno, strerror(errno));
			return -1;
                }

                it.it_interval.tv_sec = 10;
                it.it_interval.tv_nsec = 0;
                it.it_value.tv_sec = 1;
                it.it_value.tv_nsec = 0;
                
                if(timer_settime(timerid, 0, &it, NULL) == -1){
                        printf("Error %d (%s) setting timer\n", errno, strerror(errno));
			return -1;

                }
                
		while(1){
                        connection_handler(sockfd, pack);
			if(terminate){
				break;
			}
		}
                //close connection
                close(sockfd);

                //free memory
                free_package(pack);
                freeaddrinfo(servinfo);

                //remove buffer file
                remove(OUTPUT);
	}
	return 0;
}
