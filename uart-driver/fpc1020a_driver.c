#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/serdev.h>
#include <linux/types.h>
#include <linux/mod_devicetable.h>
#include <linux/property.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/string.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/wait.h>

#include "fpc1020a_driver.h"

DECLARE_WAIT_QUEUE_HEAD(wait_queue_tx);

/* Variables for module init */
dev_t dev = 0;
static struct class *dev_class;
static struct cdev fpc1020a_cdev;
int temp_flag = 0;

unsigned short TX_BUF_SIZE = 8;
unsigned short i = 0, j = 0, pos = 0;

// FPC1020A device struct
struct uart_dev {
	struct serdev_device *serdev;
	uint8_t *tx_buf; //fpc1020a tx buf is pi rx buf
	uint8_t *rx_buf; //fpc1020a rx buf is pi tx buf
	unsigned int tx_count;
	char tx_end_flag;
	unsigned int u_id;
	unsigned int u_count;
	unsigned char rtflag;
	uint8_t img_matrix[80][40];
};
struct uart_dev *fpc1020a = NULL;

/* Declare the probe and remove functions */
static int fpc1020a_probe(struct serdev_device *serdev);
static void fpc1020a_remove(struct serdev_device *serdev);

/* Init + Exit + File ops function prototypes */
static int __init fpc1020a_driver_init(void);
static void __exit fpc1020a_driver_exit(void);
static int fpc1020a_open(struct inode *inode, struct file *file);
static int fpc1020a_release(struct inode *inode, struct file *file);
static ssize_t fpc1020a_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t fpc1020a_write(struct file *filp, const char *buf, size_t len, loff_t *off);
static long fpc1020a_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/* FPC1020A module functionality */
static char fpc1020a_gen_check_sum(unsigned char wLen, unsigned char *ptr);
static void fpc1020a_send_package(unsigned char wLen, unsigned char *ptr);
static char fpc1020a_check_package(unsigned char cmd);
static char fpc1020a_search(void);
//static void fpc1020a_identify(unsigned int u_id);
static void fpc1020a_enroll1(unsigned int u_id);
static void fpc1020a_enroll2(unsigned int u_id);
static void fpc1020a_enroll3(unsigned int u_id);
static char fpc1020a_enroll(unsigned int u_id);
static char fpc1020a_delete_all(void);
static char fpc1020a_delete_user(unsigned int u_id);
static char fpc1020a_user_count(void);
//static char fpc1020a_get_user_id(void);
static char fpc1020a_get_image(void);

static struct of_device_id fpc1020a_of_match_ids[] = {
	{
		.compatible = "fpc,fpc1020a",
	}, { /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, fpc1020a_of_match_ids);

/* Callback is called whenever a character is received */
static int uart_receive(struct serdev_device *serdev, const uint8_t *buff, size_t size)
{
	// Copying received character to fpc1020a TX buffer (pi's RX) and count
	unsigned int i = 0;
	memcpy(fpc1020a->tx_buf + fpc1020a->tx_count, buff, size);
	fpc1020a->tx_count++;
	if(fpc1020a->tx_count == TX_BUF_SIZE) {
		printk(KERN_INFO "pi: RX received %d bytes:\n", fpc1020a->tx_count);
		for(i = 0; i < fpc1020a->tx_count; i++) {
			printk(KERN_CONT "0x%02x ", *(fpc1020a->tx_buf + i));
		}
		fpc1020a->tx_end_flag = 1;
		wake_up_interruptible(&wait_queue_tx);
	}
	return size;
}

static const struct serdev_device_ops serdev_ops = {
	.receive_buf = uart_receive,
};

static struct serdev_device_driver fpc1020a_driver = {
	.probe = fpc1020a_probe,
	.remove = fpc1020a_remove,
	.driver = {
		.name = "fpc1020a",
		.owner = THIS_MODULE,
		.of_match_table = fpc1020a_of_match_ids,
	},
};

/* This function is called on loading the driver */
static int fpc1020a_probe(struct serdev_device *serdev)
{
	printk(KERN_INFO "IN PROBE FUNCTION\n");
	printk(KERN_INFO "fpc1020a: Hello Kernel World!\n");
	printk(KERN_INFO "%s, %d\n", __func__, __LINE__);
	
	// Memory allocating our uart device on the heap
	// NOTE: 'devm_kzalloc' automatically free memory when unload our device
	fpc1020a = devm_kzalloc(&serdev->dev, sizeof(struct uart_dev), GFP_KERNEL);
	if(!fpc1020a){
		printk(KERN_ERR "UART device memory allocation failed\n");
		return -1;
	}
	printk("fpc1020a: Initialized struct uart_dev with size of %d bytes\n", sizeof(struct uart_dev));
	fpc1020a->serdev = serdev;
	serdev_device_set_drvdata(serdev, fpc1020a);
	
	serdev_device_set_client_ops(fpc1020a->serdev, &serdev_ops);
	if(serdev_device_open(fpc1020a->serdev) != 0){
		printk(KERN_ERR "Cannot open serial device\n");
		return -1;
	}
	serdev_device_set_baudrate(fpc1020a->serdev, 19200);
	serdev_device_set_flow_control(fpc1020a->serdev, false);
	serdev_device_set_parity(fpc1020a->serdev, SERDEV_PARITY_NONE);
	
	// Buffer memory allocation
	fpc1020a->rx_buf = kzalloc(8 * sizeof(uint8_t), GFP_KERNEL);
	if(fpc1020a->rx_buf == NULL){
		printk(KERN_ERR "kzalloc for UART device RX buffer failed\n");
		return -1;
	}
	
	fpc1020a->tx_buf = kzalloc(8 * sizeof(uint8_t), GFP_KERNEL);
	if(fpc1020a->tx_buf == NULL){
		printk(KERN_ERR "kzalloc for UART device TX buffer failed\n");
		return -1;
	}
	
	memset(fpc1020a->rx_buf, 0, 8);
	memset(fpc1020a->tx_buf, 0, 8);
	
	fpc1020a->tx_count = 0;
	fpc1020a->tx_end_flag = 0; // Not end = 0, end = 1
	
	return 0;
}

/* This function is called on unloading the driver */
static void fpc1020a_remove(struct serdev_device *serdev)
{
	printk(KERN_INFO "IN REMOVE FUNCTION\n");
	printk(KERN_INFO "%s, %d\n", __func__, __LINE__);
	fpc1020a = serdev_device_get_drvdata(serdev);
	if(!fpc1020a){
		printk("Get private data failure\n");
	} else {
		kfree(fpc1020a->tx_buf);
		kfree(fpc1020a->rx_buf);
		printk(KERN_INFO "fpc1020a: Goodbye Kernel World!\n");
		serdev_device_close(fpc1020a->serdev);
	}	
}

/* File operations structure */
static struct file_operations fops = {
	.owner			= THIS_MODULE,
	.read			= fpc1020a_read,
	.write			= fpc1020a_write,
	.open			= fpc1020a_open,
	.release		= fpc1020a_release,
	.unlocked_ioctl	= fpc1020a_ioctl,
};

static int fpc1020a_open(struct inode *inode, struct file *file){
	printk(KERN_INFO "DEVICE FILE OPENED!\n");
	return 0;
}

static int fpc1020a_release(struct inode *inode, struct file *file){
	printk(KERN_INFO "DEVICE FILE CLOSED!\n");
	return 0;
}

static ssize_t fpc1020a_read(struct file *filp, char __user *buf, size_t len, loff_t *off){
	printk(KERN_INFO "DEVICE FILE READ!\n");
	return 0;
}

static ssize_t fpc1020a_write(struct file *filp, const char *buf, size_t len, loff_t *off){
	printk(KERN_INFO "DEVICE FILE WRITE!\n");
	return len;
}

static long fpc1020a_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
		case GET_RT_FLAG:
			if(copy_to_user((unsigned char*) arg, &(fpc1020a->rtflag), sizeof(fpc1020a->rtflag))) {
				printk(KERN_ERR "fpc1020a: ERROR Couldn't send return flag to user space\n");
			}
			printk(KERN_INFO "fpc1020a: Return flag sent to user space successfully\n");
			printk(KERN_INFO "Return flag: value = 0x%02x\n", fpc1020a->rtflag);
			break;
		
		case ADD_NEW_USER:
			fpc1020a->tx_count = 0;
			fpc1020a->u_id = 0;
			if(copy_from_user(&(fpc1020a->u_id), (unsigned int*) arg, sizeof(fpc1020a->u_id))) {
				printk(KERN_ERR "fpc1020a: ERROR Couldn't get user ID input!\n");
			}
			printk(KERN_INFO "fpc1020a: User ID input = %d\n", fpc1020a->u_id);
			if(fpc1020a->u_id > 0 && fpc1020a->u_id < 99) {
				fpc1020a_enroll(fpc1020a->u_id);
			} else {
				printk("User ID invalid\n");
			}
			
			if(fpc1020a->rtflag == ACK_SUCCESS) {
				printk(KERN_INFO "fpc1020a: Adding new user succeeded\n");
			} else {
				printk(KERN_ERR "fpc1020a: Adding new user failed\n");
			}
			break;
			
		case FP_SEARCH:
			fpc1020a->tx_count = 0;
			fpc1020a->u_id = 0;
			printk("fpc1020a: Searching if fingerprint exists or not\n");
			fpc1020a_search();
			if(fpc1020a->rtflag == ACK_SUCCESS) {
				printk(KERN_INFO "fpc1020a: Found a fingerprint with user ID of %d\n", fpc1020a->u_id);
				if(copy_to_user((unsigned int*) arg, &(fpc1020a->u_id), sizeof(fpc1020a->u_id))) {
					printk(KERN_ERR "fpc1020a: Couldn't send ID to user space\n");
				}
			} else {
				printk(KERN_INFO "fpc1020a: Fingerprint does not exist!\n");
			}
			break;
			
		case USER_COUNT:
			fpc1020a->tx_count = 0;
			printk("fpc1020a: Counting users\n");
			fpc1020a_user_count();
			if(fpc1020a->rtflag == ACK_SUCCESS) {
				printk(KERN_INFO "fpc1020a: User count: %d\n", fpc1020a->u_count);
				if(copy_to_user((unsigned int*) arg, &(fpc1020a->u_count), sizeof(fpc1020a->u_count))) {
					printk(KERN_ERR "fpc1020a: Couldn't send count to user space\n");
				}
			} else {
				printk(KERN_INFO "fpc1020a: Couldn't retrieve user count\n");
			}
			break;
			
		case DELETE_A_USER:
			fpc1020a->tx_count = 0;
			if(copy_from_user(&(fpc1020a->u_id), (unsigned int*) arg, sizeof(fpc1020a->u_id))){
				printk(KERN_ERR "Data write: ERROR!\n");
			}
			printk(KERN_INFO "fpc1020a: User ID to delete = %d\n", fpc1020a->u_id);
			if(fpc1020a->u_id > 0 && fpc1020a->u_id < 99) {
				fpc1020a_delete_user(fpc1020a->u_id);
			} else {
				printk("User ID invalid\n");
			}
			break;
			
		case DELETE_ALL_USER:
			fpc1020a->tx_count = 0;
			printk("fpc1020a: Deleting all users\n");
			fpc1020a_delete_all();
			break;
			
		case GET_FP_IMAGE:
			fpc1020a->tx_count = 0;
			// Reallocate memory for storing image data
			TX_BUF_SIZE = 3211;
			fpc1020a->tx_buf = krealloc(fpc1020a->tx_buf, 3211 * sizeof(uint8_t), GFP_KERNEL);

			printk("fpc1020a: Getting user's fingeprint image\n");
			fpc1020a_get_image();
			if(fpc1020a->rtflag == ACK_SUCCESS) {
				printk(KERN_INFO "fpc1020a: Got the fingerprint image\n");
				if(copy_to_user((uint8_t*) arg, &(fpc1020a->img_matrix), sizeof(fpc1020a->img_matrix))) {
					printk(KERN_ERR "fpc1020a: Couldn't send image data to user space\n");
				} else {
					printk(KERN_INFO "fpc1020a: Sent image data to user space\n");
				}
			} else {
				printk(KERN_ERR "fpc1020a: Couldn't retrieve image data\n");
			}
			
			TX_BUF_SIZE = 8;
			fpc1020a->tx_buf = krealloc(fpc1020a->tx_buf, 8 * sizeof(uint8_t), GFP_KERNEL);
			break;
		
		default:
			printk(KERN_INFO "Default\n");
			break;
	}
	return 0;
}

/* FPC1020A functionality implementation */
static char fpc1020a_gen_check_sum(unsigned char wLen, unsigned char *ptr)
{
	unsigned char i, temp = 0;
	
	for(i = 0; i < wLen; i++) {
		temp ^= *(ptr + i);
	}
	return temp;
}

static void fpc1020a_send_package(unsigned char wLen, unsigned char *ptr)
{
	unsigned int i = 0, len = 0;
	
	*(fpc1020a->rx_buf) = DATA_START;
	
	for(i = 0; i < wLen; i++) {
		*(fpc1020a->rx_buf + 1 + i) =  *(ptr + i);
	}
	
	*(fpc1020a->rx_buf + wLen + 1) = fpc1020a_gen_check_sum(wLen, ptr);
	*(fpc1020a->rx_buf + wLen + 2) = DATA_END;
	len = wLen + 3;
	
	// Add flag and UART send code here
	printk("pi: TX buffer:\n");
	for(i = 0; i < len; i++) {
		printk(KERN_CONT "0x%02x ", *(fpc1020a->rx_buf + i));
	}
	serdev_device_write_buf(fpc1020a->serdev, fpc1020a->rx_buf, len);
	printk(KERN_INFO "pi: TX buffer sent to FPC1020A\n");
}

static char fpc1020a_check_package(unsigned char cmd)
{
	fpc1020a->rtflag = ACK_FAIL;
	
	wait_event_interruptible(wait_queue_tx, fpc1020a->tx_end_flag == 1);
	
	switch(cmd){
		case CMD_ENROLL1:
		case CMD_ENROLL2:
		case CMD_ENROLL3:
			if(ACK_SUCCESS == *(fpc1020a->tx_buf + 4)){
				fpc1020a->rtflag = ACK_SUCCESS;
			} else if(ACK_USER_EXIST == *(fpc1020a->tx_buf + 4)){
				fpc1020a->rtflag = ACK_USER_EXIST;
				msleep(500);
			} else if(ACK_USER_OCCUPIED == *(fpc1020a->tx_buf + 4)){
				fpc1020a->rtflag = ACK_USER_OCCUPIED;
				msleep(500);
			} else if(ACK_TIMEOUT == *(fpc1020a->tx_buf + 4)){
				fpc1020a->rtflag = ACK_TIMEOUT;
				msleep(500);
			}
			break;
		
		case CMD_DELETE_USER:
			if(ACK_SUCCESS == *(fpc1020a->tx_buf + 4)){
				fpc1020a->rtflag = ACK_SUCCESS;
			}
			break;
			
		case CMD_DELETE_ALL:
			if(ACK_SUCCESS == *(fpc1020a->tx_buf + 4)){
				fpc1020a->rtflag = ACK_SUCCESS;
			}
			break;
			
		case CMD_IDENTIFY:
			if(ACK_SUCCESS == *(fpc1020a->tx_buf + 4)){
				fpc1020a->rtflag = ACK_SUCCESS;
			}
			break;
		
		case CMD_USER_COUNT:
			if(ACK_SUCCESS == *(fpc1020a->tx_buf + 4)){
				fpc1020a->rtflag = ACK_SUCCESS;
				fpc1020a->u_count = *(fpc1020a->tx_buf + 3);
			}
			break;
		
		case CMD_SEARCH:
			if((1 == *(fpc1020a->tx_buf + 4)) || (2 == *(fpc1020a->tx_buf + 4)) || (3 == *(fpc1020a->tx_buf + 4))){
				fpc1020a->rtflag = ACK_SUCCESS;
				fpc1020a->u_id = *(fpc1020a->tx_buf + 3);
			}
			break;
			
		case CMD_GET_USER_ID:
			if(ACK_SUCCESS == *(fpc1020a->tx_buf + 4)) {
				fpc1020a->rtflag = ACK_SUCCESS;
				fpc1020a->u_id = *(fpc1020a->tx_buf + 3);
			}
			break;
		
		case CMD_GET_IMAGE:
			if(ACK_SUCCESS == *(fpc1020a->tx_buf + 4)) {
				fpc1020a->rtflag = ACK_SUCCESS;
				pos = 9;
				for(i = 0; i < 80; i++) {
					for(j = 0; j < 40; j++) {
						fpc1020a->img_matrix[i][j] = *(fpc1020a->tx_buf + pos);
						pos++;
					}
				}
			}
			break;
			
		default:
			break;
	}
	fpc1020a->tx_end_flag = 0; // TX end flag reset
	return fpc1020a->rtflag;
}

static char fpc1020a_search(void)
{
	unsigned char buf[5];
	
	*buf = CMD_SEARCH;
	*(buf + 1) = 0x00;
	*(buf + 2) = 0x00;
	*(buf + 3) = 0x00;
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);
	return fpc1020a_check_package(CMD_SEARCH);
}
/*
static void fpc1020a_identify(unsigned int u_id)
{
	unsigned char buf[5];
	
	*buf = CMD_IDENTIFY;
	*(buf + 1) = u_id >> 8;
	*(buf + 2) = u_id & 0xff;
	*(buf + 3) = 0x00;
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);
}
*/
static void fpc1020a_enroll1(unsigned int u_id)
{
	unsigned char buf[5];
	
	*buf = CMD_ENROLL1;
	*(buf + 1) = u_id >> 8;
	*(buf + 2) = u_id & 0xff;
	*(buf + 3) = 1;				// User permission
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);
}

static void fpc1020a_enroll2(unsigned int u_id)
{
	unsigned char buf[5];
	
	*buf = CMD_ENROLL2;
	*(buf + 1) = u_id >> 8;
	*(buf + 2) = u_id & 0xff;
	*(buf + 3) = 1;				// User permission
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);	
}

static void fpc1020a_enroll3(unsigned int u_id)
{
	unsigned char buf[5];
	
	*buf = CMD_ENROLL3;
	*(buf + 1) = u_id >> 8;
	*(buf + 2) = u_id & 0xff;
	*(buf + 3) = 1;				// User permission
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);	
}

static char fpc1020a_enroll(unsigned int u_id)
{
	printk("fpc1020a: Adding new fingerprint started\n");
	fpc1020a->tx_count = 0;
	fpc1020a_enroll1(u_id);
	fpc1020a_check_package(CMD_ENROLL1);
	printk("1st Enroll\n");
	if(fpc1020a->rtflag != ACK_SUCCESS) {
		return fpc1020a->rtflag;
	}
	printk("Remove finger for 2nd enroll\n");
	msleep(1000);
	
	fpc1020a->tx_count = 0;
	fpc1020a_enroll2(u_id);
	fpc1020a_check_package(CMD_ENROLL2);
	printk("2nd Enroll\n");
	if(fpc1020a->rtflag != ACK_SUCCESS) {
		return fpc1020a->rtflag;
	}
	printk("Remove finger for 3rd enroll\n");
	msleep(1000);
	
	fpc1020a->tx_count = 0;
	fpc1020a_enroll3(u_id);
	fpc1020a_check_package(CMD_ENROLL3);
	printk("3rd Enroll\n");
	if(fpc1020a->rtflag != ACK_SUCCESS) {
		return fpc1020a->rtflag;
	}
	printk("Scan Finished. Remove finger\n");
	msleep(1000);
	return fpc1020a->rtflag;
}

static char fpc1020a_delete_all(void)
{
	unsigned char buf[5];
	
	*buf = CMD_DELETE_ALL;
	*(buf + 1) = 0x00;
	*(buf + 2) = 0x00;
	*(buf + 3) = 0x00;
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);
	return fpc1020a_check_package(CMD_DELETE_ALL);
}

static char fpc1020a_delete_user(unsigned int u_id)
{
	unsigned char buf[5];
	
	*buf = CMD_DELETE_USER;
	*(buf + 1) = u_id >> 8;
	*(buf + 2) = u_id & 0xFF;
	*(buf + 3) = 0x00;
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);
	return fpc1020a_check_package(CMD_DELETE_USER);
}

static char fpc1020a_user_count(void)
{
	unsigned char buf[5];
	
	*buf = CMD_USER_COUNT;
	*(buf + 1) = 0x00;
	*(buf + 2) = 0x00;
	*(buf + 3) = 0x00;
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);
	return fpc1020a_check_package(CMD_USER_COUNT);
}
/*
static char fpc1020a_get_user_id(void)
{
	unsigned char buf[5];
	
	*buf = CMD_GET_USER_ID;
	*(buf + 1) = 0x00;
	*(buf + 2) = 0x00;
	*(buf + 3) = 0x00;
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);
	return fpc1020a_check_package(CMD_GET_USER_ID);
}
*/
static char fpc1020a_get_image(void)
{
	unsigned char buf[5];
	
	*buf = CMD_GET_IMAGE;
	*(buf + 1) = 0x00;
	*(buf + 2) = 0x00;
	*(buf + 3) = 0x00;
	*(buf + 4) = 0x00;
	
	fpc1020a_send_package(5, buf);
	return fpc1020a_check_package(CMD_GET_IMAGE);
}

static int __init fpc1020a_driver_init(void)
{
	temp_flag = alloc_chrdev_region(&dev, 0, 1, "fpc1020_driver");

	if(temp_flag < 0){
		printk(KERN_ERR "Cannot allocate major number for device\n");
		return -1;
	}
	printk(KERN_NOTICE "Printing device major and minor\n");
	printk(KERN_INFO "Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));
	
	cdev_init(&fpc1020a_cdev, &fops);
	temp_flag = cdev_add(&fpc1020a_cdev, dev, 1); 
	if(temp_flag < 0){
		printk(KERN_ERR "Cannot add the device to the system\n");
		goto r_class;
	}
	
	dev_class = class_create(THIS_MODULE, "fpc1020a_class");
	/*
	if(IS_ERR(class_create(THIS_MODULE, "fpc1020a_class"))){
		printk(KERN_ERR "Cannot create the struct class for the device\n");
		goto r_class;
	*/
	
	if(IS_ERR(device_create(dev_class, NULL, dev, NULL, "fpc1020a_dev"))){
		printk(KERN_ERR "Cannot create the device\n");
		goto r_device;
	}
	
	temp_flag = serdev_device_driver_register(&fpc1020a_driver);
	if(temp_flag){
		printk(KERN_ERR "fpc1020a - Error! Could not load driver\n");
		return -1;
	}
	
	printk("fpc1020a - Loaded the driver...\n");
	return 0;

r_device:
	class_destroy(dev_class);
r_class:
	unregister_chrdev_region(dev, 1);
	return -1;
}

static void __exit fpc1020a_driver_exit(void)
{
	fpc1020a->tx_end_flag = 1;
	
	serdev_device_driver_unregister(&fpc1020a_driver);
	device_destroy(dev_class, dev);
	class_destroy(dev_class);
	cdev_del(&fpc1020a_cdev);
	unregister_chrdev_region(dev, 1);
	printk("fpc1020a - Unload driver\n");
}

module_init(fpc1020a_driver_init);
module_exit(fpc1020a_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("SIPLAB");
MODULE_DESCRIPTION("A simple loopback driver for an UART port");
MODULE_VERSION("1.0");
