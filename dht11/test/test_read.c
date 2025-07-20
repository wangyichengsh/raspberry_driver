#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

struct dht11_data{
	int temp;
	int humi;
};



int main(int argc, char *argv[]){
	int fd;
	struct dht11_data data;
	memset(&data, 0, sizeof(data));
	sleep(2);
	fd = open("/dev/dht11", O_RDONLY);
	if(fd<0){
		printf("open dht11 failed\n");
		printf("err: %d\n", fd);
		close(fd);
		return -1;
	}
	int ret = 0;

	ret = read(fd, &data, sizeof(data));
	if(ret<0){
		printf("read failed\n");
		printf("err: %d\n", ret);
		close(fd);
		return -1;
	}
	printf("temp: %d\n", data.temp);
	printf("humi: %d\n", data.humi);
	close(fd);
	return 0;
}
