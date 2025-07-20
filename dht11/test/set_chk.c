#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdbool.h>

#define SET_CHECKSUM _IOW('D', 1, bool)

int main(int argc, char *argv[]){
	int fd;
	fd = open("/dev/dht11", O_RDONLY);
	if(fd<0){
		printf("open dht11 failed\n");
		printf("err: %d\n", fd);
		close(fd);
		return -1;
	}
	int ret = 0;
	
	int flag = atoi(argv[1]); 
	if(flag == 0){
		ret = ioctl(fd, SET_CHECKSUM, false);
	}else{
		ret = ioctl(fd, SET_CHECKSUM, true);
	}
	if(ret<0){
		printf("set checksum failed\n");
		printf("err: %d\n", ret);
		close(fd);
		return -1;
	}
	close(fd);
	printf("checksum setted sucessfully\n");
	return 0;
}
