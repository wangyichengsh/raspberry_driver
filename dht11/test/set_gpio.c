#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define SET_GPIO _IOW('D', 0, int)

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
	
	int pin_gpio = atoi(argv[1]); 

	ret = ioctl(fd, SET_GPIO, pin_gpio);
	if(ret<0){
		printf("set gpio failed\n");
		printf("err: %d\n", ret);
		close(fd);
		return -1;
	}
	close(fd);
	printf("gpio has been setted\n");
	return 0;
}
