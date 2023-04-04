/*

    This input kernel module is written for the COP4600 class for the PA2

    This was written by Yohan Hmaiti, Anna Zheng, and Alyssa Yeekee

*/

#include <linux/uaccess.h>

#include <linux/vmalloc.h>

#include <linux/mutex.h>

#include <linux/init.h>

#include <linux/device.h>

#include <linux/module.h>

#include <linux/kernel.h>

#include <linux/fs.h>

#define MAXLENGTH 1024

#define DEVICE_NAME "pa2OS_in"

#define CLASS_NAME "pa2OS_inModule"

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Yohan Hmaiti, Anna Zheng, and Alyssa Yeekee");

MODULE_DESCRIPTION("Input module for pa2OS - COP4600");

MODULE_VERSION("0.1");

struct sharedMemory

{

    char data[1024];

    int length;

    int _isFull;

    int _isEmpty;
};

DEFINE_MUTEX(sharedData_access_mutex);

static int major_number;

static int openingCount_DeviceInput = 0;

static int messageSize;

static struct class *pa2OSInputClass = NULL;

static struct device *pa2OSInputDevice = NULL;

struct sharedMemory *dataShared;

EXPORT_SYMBOL(dataShared);

EXPORT_SYMBOL(sharedData_access_mutex);

char helper[1024];

/*since technically this is just an input module we will just have a write procedure here*/

static ssize_t write(struct file *, const char *, size_t, loff_t *);

static int open(struct inode *, struct file *);

static int close(struct inode *, struct file *);

static struct file_operations fops =

    {

        .owner = THIS_MODULE,

        .open = open,

        .release = close,

        .write = write,

};

/*Some of the content here was written with reference to google, and ChatGPT including how to write the prototype*/

/*We also relied on some content from the first programming assignment here*/

static int __init initialize(void)

{

    printk(KERN_INFO "lkm_pa2-in init function called -> installing the input module....\n");

    major_number = register_chrdev(0, DEVICE_NAME, &fops);

    if (major_number < 0)

    {

        printk(KERN_ALERT "lkm_pa2-in could not register number.\n");

        return major_number;
    }

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> Registered correctly with major number %d\n", major_number);

    pa2OSInputClass = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(pa2OSInputClass))

    {

        unregister_chrdev(major_number, DEVICE_NAME);

        printk(KERN_ALERT "Device class failed to be registered (failed...).\n");

        return PTR_ERR(pa2OSInputClass);
    }

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> Device class registered correctly.\n");

    pa2OSInputDevice = device_create(pa2OSInputClass, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);

    if (IS_ERR(pa2OSInputDevice))

    {

        class_destroy(pa2OSInputClass);

        unregister_chrdev(major_number, DEVICE_NAME);

        printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> Device was not created successfully.... Failed to create the device.\n");

        return PTR_ERR(pa2OSInputDevice);
    }

    mutex_init(&sharedData_access_mutex);

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> mutex initialized successfully....\n");

    dataShared = vmalloc(sizeof(struct sharedMemory));

    if (dataShared == NULL)

    {

        printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> Failed to allocate memory for the shared memory.\n");

        return -ENOMEM;
    }

    memset(dataShared, 0, sizeof(struct sharedMemory));

    dataShared->_isEmpty = 1;

    dataShared->_isFull = 0;

    dataShared->length = 0;

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> Device was created successfully....\n");

    // now we simply await the lock and then start the simulation

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> Waiting for the lock to write to the shared memory....\n");

    return 0;
}

static int open(struct inode *inodep, struct file *filep)

{

    if (openingCount_DeviceInput > 0)

    {

        printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> Device is already open.\n");

        return -EBUSY;
    }

    openingCount_DeviceInput++;

    try_module_get(THIS_MODULE);

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> opened now.\n");

    return 0;
}

static void __exit destroy(void)

{

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> removing module.\n");

    vfree(dataShared);

    mutex_destroy(&sharedData_access_mutex);

    device_destroy(pa2OSInputClass, MKDEV(major_number, 0));

    class_unregister(pa2OSInputClass);

    class_destroy(pa2OSInputClass);

    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> Goodbye from the LKM!\n");

    unregister_chrdev(major_number, DEVICE_NAME);

    return;
}

static int close(struct inode *inodep, struct file *filep)

{

    if (openingCount_DeviceInput <= 0)

    {

        printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> Device is already closed, we will not close it again.\n");

        return -EBUSY;
    }

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> input device closed.\n");

    openingCount_DeviceInput--;

    module_put(THIS_MODULE);

    return 0;
}

/*some of the content here was obtained by reading the kernel coding documentation, chatGPT and google*/

static ssize_t write(struct file *filep, const char *buffer, size_t len, loff_t *offset)

{

    if (len < 0)
    {

        printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> message length is less than 0, we won't write anything.\n");

        return len;
    }

    if (len > MAXLENGTH - 1)

    {

        printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> The length of the message passed is bigger than the max size allowed, we will only write and store up to 1KB (1024).\n");

        len = 1024 - 1;
    }

    // print that we are waiting for the lock
    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> waiting for the lock to write to the shared data space....\n");

    int lenHelper = snprintf(helper, sizeof(helper), "%s", buffer);

    if (lenHelper < 0)

    {

        printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> Failed to copy the message to the helper buffer.\n");

        return lenHelper;
    }

    if (lenHelper > MAXLENGTH - 1)

    {

        printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> The length of the message passed is bigger than the max size allowed, we will only write and store up to 1KB (1024).\n");

        lenHelper = 1024 - 1;

        helper[lenHelper] = '\0';
    }

    printk(KERN_INFO "waiting for the lock still, we will write the message soon......");

    if (!mutex_trylock(&sharedData_access_mutex))

    {

        printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> try lock failed, we will wait more till the lock is acquirable and we can access the shared mem to write....\n");

        return -EBUSY;
    }

    printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> Lock acquired, in the critical section now.....\n\n");

    // allocate memoryfor the struct first

    memset(dataShared, 0, sizeof(struct sharedMemory));

    /*     dataShared->_isEmpty = 0;

        dataShared->_isFull = 1; */

    dataShared->length = lenHelper;

    /* based on the documentation

    The snprintf function returns the number of characters that would have been written to the buffer

    if there were no size limit (excluding the terminating null character).*/

    dataShared->length = snprintf(dataShared->data, MAXLENGTH, "%s", helper);

    dataShared->data[dataShared->length] = '\0';

    if (dataShared->length < 0)

    {

        printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> Failed to copy the message successfully.....\n");

        return dataShared->length;
    }

    mutex_unlock(&sharedData_access_mutex);

    printk(KERN_ALERT "lkm_pa2-in (pa2OS_inModule) -> Lock released, out of the critical section now.....\n\n");

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> received %d from the user, we wrote %d.....\n", (int)len, (int)lenHelper);

    // awaiting the output to be printed now

    printk(KERN_INFO "lkm_pa2-in (pa2OS_inModule) -> now the output device will take care of stuff we wait for the print and lock release from the output process....\n");

    return len;
}

module_init(initialize);

module_exit(destroy);