#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kdev_t.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define SET_GPIO _IOW('D', 0, int)
#define SET_CHECKSUM _IOW('D', 1, bool)

static int pin_gpio = 538; 

struct dht11_sensor {
	dev_t dev_num;
	struct cdev dht11_cdev;
	struct class *dht11_class;
	struct device *dht11_device;
	struct gpio_desc *dht11_gpio;
	bool checksum;
};

struct dht11_sensor *dht11;

struct dht11_data {
	int temp;
	int humi;
};

struct dht11_data data;

unsigned char read_bite(void){
	unsigned char bite;
	while(!gpiod_get_value(dht11->dht11_gpio));
	// udelay(54);
	udelay(40);
	bite = gpiod_get_value(dht11->dht11_gpio);
	if(bite){
		// while(gpiod_get_value(dht11->dht11_gpio));
		udelay(60);
	}
	return bite;
}

int read_byte(void){
	int res = 0;
	for(int i=0;i<8;++i){
	       	res <<= 1;	
		res |= read_bite();
	}
	return res;
}

void cal_humi(int h, int l){
	data.humi = h;
}

void cal_temp(int h, int l){
	bool flag = false;
	if(1<<7&l){
		flag = true;
		l &= 0x7f;
	}
	data.temp = h;
	if(flag){
		data.temp = -data.temp;
	}
}

bool check_data(int h_h, int h_l, int t_h, int t_l, int chk){
	return h_h+h_l+t_h+t_l==chk;
}

int dht11_readdata(void){
	int humi_h,humi_l,temp_h,temp_l,check;
        memset(&data, 0, sizeof(data));	
	gpiod_set_value(dht11->dht11_gpio, 0);
	mdelay(20);
	gpiod_set_value(dht11->dht11_gpio, 1);
	udelay(13);
	gpiod_direction_input(dht11->dht11_gpio);
	while(gpiod_get_value(dht11->dht11_gpio));
	while(!gpiod_get_value(dht11->dht11_gpio));
	while(gpiod_get_value(dht11->dht11_gpio));
	humi_h = read_byte();
	printk("humi_h: %x \n", humi_h);
	humi_l = read_byte();
	printk("humi_l: %x \n", humi_l);
	temp_h = read_byte();
	printk("temp_h: %x \n", temp_h);
	temp_l = read_byte();
	printk("temp_l: %x \n", temp_l);
	check = read_byte();
	printk("check: %x \n", check);
	udelay(70);
	printk("dht11 40 bits received\n");
	gpiod_direction_output(dht11->dht11_gpio, 1);
	cal_humi(humi_h, humi_l);
	cal_temp(temp_h, temp_l);
	if(!dht11->checksum){
		return 0;
	}
	if(check_data(humi_h, humi_l, temp_h, temp_l, check)){
		return 0;
	}else{
		printk("chechsum failed\n");
		return -1;
	}
}

int dht11_open(struct inode *inode, struct file *file){
	return 0;
}
ssize_t dht11_read(struct file *file, char __user *buf, size_t size, loff_t *offs){
	int ret; 
	if(dht11->dht11_gpio != NULL){
		ret = dht11_readdata();
	}else{
		printk("gpio inititalize failed\n");
		return -1;
	}
	if(ret<0){
		printk("dht11 read failed\n");
		return -1;
	}
	if(copy_to_user(buf, &data, sizeof(data))){
		printk("copy to user failed\n");
		return -1;
	}
	return sizeof(data);
}

int dht11_release(struct inode *inode, struct file *file){
	return 0;
}

long dht11_ioctl(struct file *file, unsigned int cmd, unsigned long arg){
	if(cmd==SET_GPIO){
		if(dht11->dht11_gpio && !IS_ERR(dht11->dht11_gpio)){
			gpiod_put(dht11->dht11_gpio);
		}
		dht11->dht11_gpio = gpio_to_desc(arg);
		if(dht11->dht11_gpio==NULL || IS_ERR(dht11->dht11_gpio)){
			printk("set gpio failed\n");
			return -1; 
		}		
		gpiod_direction_output(dht11->dht11_gpio, 1);
	}
	if(cmd==SET_CHECKSUM){
		dht11->checksum = arg;	
	}
	return 0;
}

struct file_operations dht11_fops = {
	.owner = THIS_MODULE,
	.open = dht11_open,
	.read = dht11_read,
	.unlocked_ioctl = dht11_ioctl,
	.release = dht11_release,	
};  


static int __init dht11_init(void){
	
	int ret = 0;
	
	dht11 = kzalloc(sizeof(struct dht11_sensor), GFP_KERNEL);
	if(dht11==NULL){
		printk("kzalloc error \n");
		ret = -ENOMEM;
		goto error_0;
	}

	ret = alloc_chrdev_region(&dht11->dev_num, 0, 1, "dht11");
	if(ret<0){
		printk("alloc chrdev error\n");
		ret = -EAGAIN;
		goto error_1;
	}
	
	cdev_init(&dht11->dht11_cdev, &dht11_fops);		
	dht11->dht11_cdev.owner = THIS_MODULE;
	cdev_add(&dht11->dht11_cdev, dht11->dev_num, 1);

	dht11->dht11_class = class_create("senor");
	if(IS_ERR(dht11->dht11_class)){
		printk("class create error\n");
		ret = PTR_ERR(&dht11->dht11_class);
		goto error_2;
	}

	dht11->dht11_device = device_create(dht11->dht11_class, NULL, dht11->dev_num, NULL, "dht11");
	if(IS_ERR(dht11->dht11_device)){
		printk("device create error\n");
		ret = PTR_ERR(dht11->dht11_device);
		goto error_3;
	}
	
	dht11->dht11_gpio = gpio_to_desc(pin_gpio);
	if(dht11->dht11_gpio == NULL){
		ret = -EBUSY;
		goto error_4;
	}
	dht11->checksum = true;
	gpiod_direction_output(dht11->dht11_gpio, 1);
	return 0;
error_4:
	device_destroy(dht11->dht11_class, dht11->dev_num);
error_3:
	class_destroy(dht11->dht11_class);
error_2:
	cdev_del(&dht11->dht11_cdev);
	unregister_chrdev_region(dht11->dev_num, 1);
error_1:
	kfree(dht11);
	dht11 = NULL;
error_0:
	return ret;
}

static void __exit dht11_exit(void){
	if(dht11->dht11_gpio && !IS_ERR(dht11->dht11_gpio)){
		gpiod_put(dht11->dht11_gpio);
	}
	device_destroy(dht11->dht11_class, dht11->dev_num);
	class_destroy(dht11->dht11_class);
	cdev_del(&dht11->dht11_cdev);
	unregister_chrdev_region(dht11->dev_num, 1);
	kfree(dht11);
	dht11 = NULL; 
}

module_init(dht11_init);
module_exit(dht11_exit);
MODULE_LICENSE("GPL");

