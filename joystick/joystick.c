#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/i2c.h>

#define JOYSTICK_ADDR 0x50

struct iic_joystick {
	struct input_dev *input;
	struct i2c_adapter *adapter;
};

struct iic_joystick *joystick;


static void iic_joystick_scan(struct input_dev *input){
	
	uint8_t data[4] = {0,0,0,0};
	
	struct i2c_msg msgs[1] = {
    		{ .addr = JOYSTICK_ADDR, .flags = I2C_M_RD, .len = 4, .buf = data },
	};

	int ret = i2c_transfer(joystick->adapter, msgs, 1);
	if(ret<0){
		printk("i2c transfer err: %d\n", ret);
		return;	
	}

	uint16_t x = (uint16_t)data[0] + (uint16_t)(data[1]<<8);
       	uint16_t y = (uint16_t)data[2] + (uint16_t)(data[3]<<8);	

	input_report_abs(input, ABS_X, x);
	input_report_abs(input, ABS_Y, y);
	input_sync(input);
}


static int __init joystick_init(void){
	
	joystick = kzalloc(sizeof(struct iic_joystick), GFP_KERNEL);

	joystick->adapter = i2c_get_adapter(1);
	if (!joystick->adapter) {
		printk("Unable to get i2c adapter\n");
		kfree(joystick);
		joystick = NULL;
		return -ENOMEM;
	}
	
	joystick->input = input_allocate_device();
	if (!joystick->input) {
		printk("Unable to allocate input device\n");
		i2c_put_adapter(joystick->adapter);
		kfree(joystick);
		joystick = NULL;
		return -ENOMEM;
	}
	
	input_set_abs_params(joystick->input, ABS_X, 0, 4095, 100, 100);
	input_set_abs_params(joystick->input, ABS_Y, 0, 4095, 100, 100);

	int error = input_setup_polling(joystick->input, iic_joystick_scan);
	if (error){
		input_free_device(joystick->input);
		i2c_put_adapter(joystick->adapter);
		kfree(joystick);
		joystick = NULL;
		return error;
	}

	input_set_poll_interval(joystick->input, 100);
	error = input_register_device(joystick->input);
	if (error) {
		printk("could not register input device\n");
		input_free_device(joystick->input);
		i2c_put_adapter(joystick->adapter);
		kfree(joystick);
		joystick = NULL;
		return error;
	}
	return 0;
}

static void __exit joystick_exit(void){
	input_unregister_device(joystick->input);	
	input_free_device(joystick->input);
	i2c_put_adapter(joystick->adapter);
	kfree(joystick);
	joystick = NULL;
}

module_init(joystick_init);
module_exit(joystick_exit);
MODULE_LICENSE("GPL");
