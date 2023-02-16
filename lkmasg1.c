/**
 * File:	lkmasg1.c
 * Adapted for Linux 5.15 by: John Aedo
 * Class:	COP4600-SP23
 * 
 * @modified by: Yohan Hmaiti - Alyssa YeeKee - Anna Zheng for the fulfillment of the PA1 Requirements
 */


#include <linux/module.h>	  // Core header for modules.
#include <linux/device.h>	  // Supports driver model.
#include <linux/kernel.h>	  // Kernel header for convenient functions.
#include <linux/fs.h>		  // File-system support.
#include <linux/uaccess.h>	  // User access copy function support.
#define DEVICE_NAME "lkmasg1" // Device name.
#define CLASS_NAME "char"	  ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");						 ///< The license type -- this affects available functionality
MODULE_AUTHOR("John Aedo");					 ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("lkmasg1 Kernel Module"); ///< The description -- see modinfo
MODULE_VERSION("0.1");						 ///< A version number to inform users

/**
 * Important variables that store data and keep track of relevant information.
 */
static int major_number;

static char messageBuffer[1024] = {0}; /*the size here is 1024 based on the professor's email*/

static int messageSize = 0;

static int openingCount_Device = 0;

static struct class *lkmasg1Class = NULL; ///< The device-driver class struct pointer
static struct device *lkmasg1Device = NULL; ///< The device-driver device struct pointer

/**
 * Prototype functions for file operations.
 */
static int open(struct inode *, struct file *);
static int close(struct inode *, struct file *);
static ssize_t read(struct file *, char *, size_t, loff_t *);
static ssize_t write(struct file *, const char *, size_t, loff_t *);

/**
 * File operations structure and the functions it points to.
 */
static struct file_operations fops =
{
		.owner = THIS_MODULE,
		.open = open,
		.release = close,
		.read = read,
		.write = write,
};

/**
 * Initializes module at installation
 */
int init_module(void)
{
	printk(KERN_INFO "lkmasg1: installing module.\n");

	// Allocate a major number for the device.
	major_number = register_chrdev(0, DEVICE_NAME, &fops);
	if (major_number < 0)
	{
		printk(KERN_ALERT "lkmasg1 could not register number.\n");
		return major_number;
	}
	printk(KERN_INFO "lkmasg1: registered correctly with major number %d\n", major_number);

	// Register the device class
	lkmasg1Class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(lkmasg1Class))
	{ // Check for error and clean up if there is
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(lkmasg1Class); // Correct way to return an error on a pointer
	}
	printk(KERN_INFO "lkmasg1: device class registered correctly\n");

	// Register the device driver
	lkmasg1Device = device_create(lkmasg1Class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
	if (IS_ERR(lkmasg1Device))
	{								 // Clean up if there is an error
		class_destroy(lkmasg1Class); // Repeated code but the alternative is goto statements
		unregister_chrdev(major_number, DEVICE_NAME);
		printk(KERN_ALERT "Failed to create the device\n");
		return PTR_ERR(lkmasg1Device);
	}
	printk(KERN_INFO "lkmasg1: device class created correctly\n"); // Made it! device was initialized

	return 0;
}

/*
 * Removes module, sends appropriate message to kernel
 */
void cleanup_module(void)
{
	printk(KERN_INFO "lkmasg1: removing module.\n");
	device_destroy(lkmasg1Class, MKDEV(major_number, 0)); // remove the device
	class_unregister(lkmasg1Class);						  // unregister the device class
	class_destroy(lkmasg1Class);						  // remove the device class
	unregister_chrdev(major_number, DEVICE_NAME);		  // unregister the major number
	printk(KERN_INFO "lkmasg1: Goodbye from the LKM!\n");
	unregister_chrdev(major_number, DEVICE_NAME);
	return;
}

/*
 * Opens device module, sends appropriate message to kernel
 */
static int open(struct inode *inodep, struct file *filep)
{

	if (openingCount_Device > 0)
	{
		printk(KERN_INFO "lkmasg1: device is open, so we will not open it again.\n");
		return -EBUSY;
	}
	// let's keep track of the opening count here
	openingCount_Device++;

	try_module_get(THIS_MODULE);

	printk(KERN_INFO "lkmasg1: device opened.\n");
	return 0;
}

/*
 * Closes device module, sends appropriate message to kernel
 */
static int close(struct inode *inodep, struct file *filep)
{

	// let's keep track of the opening count here

	if (openingCount_Device <= 0)
	{
		printk(KERN_INFO "lkmasg1: device is closed, so we will not close it again.\n");
		return -EBUSY;
	}
	openingCount_Device--;
	module_put(THIS_MODULE);
	printk(KERN_INFO "lkmasg1: device closed.\n");
	
	return 0;
}

/*
 * Reads from device, displays in userspace, and deletes the read data
 */
static ssize_t read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	printk(KERN_INFO "read stub");

	if (len > 1024)
	{
		printk(KERN_INFO "lkmasg1: the length of the buffer passed is bigger than 1024 (%d), so we will only read up to 1KB -> {1024}.\n", len);
		len = messageSize;
	}

	// If not enough data is available to service a read request, the driver must respond with only the amount available (including 0 bytes)
	if (len > messageSize)
	{
		printk(KERN_INFO "lkmasg1: the length of the buffer passed is bigger than the amount of data available (%d), so we will only read up to the amount of data available (%d).\n", len, messageSize);
		len = messageSize;
	}

	if(len < 0 || messageSize < 0)
	{
		printk(KERN_INFO "lkmasg1: no data available to read, length or message size are negative values\n");
		//copy_to_user(buffer, messageBuffer, len);
		return len;
	}

	if(len == 0 || messageSize == 0)
	{
		// we still respond with only the amount available (including 0 bytes)
		printk(KERN_INFO "lkmasg1: no data available to read, we just respond with the amount available (including 0 Bytes)\n");
		copy_to_user(buffer, messageBuffer, len);
		return len;
	}
	// read back the message in FIFO fashion
	// and remove the read data from the buffer as it is read
	int error = copy_to_user(buffer, messageBuffer, len);

	if (error == 0)
	{
		printk(KERN_INFO "lkmasg1: read %d bytes from the device.\n", len);
		/* messageSize = 0; */
		return len;
	}
	else
	{
		printk(KERN_INFO "lkmasg1: failed to read %d bytes from the device.\n", error);
		return -EFAULT;
	}

	/* return 0; */

	return len;
}

/*
 * Writes to the device
 */
static ssize_t write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	printk(KERN_INFO "write stub");

	if (len < 0)
	{
		printk(KERN_INFO "lkmasg1: the length of the buffer passed is negative, so we will not write anything.\n");
		return len;
	}

	// only store bytes written up to a constant buffer size of at least 1KB
	// If not enough buffer is available to store a write request, the driver must store only up to the amount available if (len > 1024)
	if (len > 1024)
	{
		printk(KERN_INFO "lkmasg1: the length of the buffer passed is bigger than 1024, so we will only write up to 1KB.\n");
		len = 1024;
	}

	// copy the data from the user space to the kernel space
	int error = copy_from_user(messageBuffer, buffer, len);

	if (error == 0)
	{
		printk(KERN_INFO "lkmasg1: received %zu characters from the user\n", len);
		messageSize = len;
		return len;
	}
	else
	{
		printk(KERN_INFO "lkmasg1: failed to receive %d characters from the user\n", error);
		return -EFAULT;
	}


	return len;
}

/*
// initialize the module
module_init(init_module);

// remove the module
module_exit(cleanup_module);
*/
// removed these two seems that they are needed! But we need more testing 