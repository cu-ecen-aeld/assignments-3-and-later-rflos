#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>

#include<syslog.h>

int main(int argc, char* argv[]){
	
	int fd;
	ssize_t nr;

	openlog(NULL, 0, LOG_USER);

	if(argc != 3){
		syslog(LOG_ERR, "Invalid Number of arguments: %d", argc);
		return 1;
	}

	fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, 0644);
	if(fd == -1){
		syslog(LOG_ERR, "Failed to open file"); 
		return 1;
	}

	nr = write(fd, argv[2], strlen(argv[2]));
	if(nr == -1){
		syslog(LOG_ERR, "Failed to write file");
		return 1;
	}
	syslog(LOG_DEBUG, "Writing %s to %s", argv[2], argv[1]);

	return 0;
}
