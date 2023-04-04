/*

    This output kernel module is written for the COP4600 class for the PA2

    This was written by Yohan Hmaiti, Anna Zheng, and Alyssa Yeekee

*/

#include <linux/init.h>

#include <linux/uaccess.h>

#include <linux/vmalloc.h>

#include <linux/mutex.h>

#include <linux/device.h>

#include <linux/module.h>

#include <linux/kernel.h>

#include <linux/fs.h>

#include "pa2OS.h"

#define MAXLENGTH 1024

#define DEVICE_NAME "pa2OS_out"

#define CLASS_NAME "pa2OS_outModule"

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Yohan Hmaiti, Anna Zheng, and Alyssa Yeekee");

MODULE_DESCRIPTION("Output module for pa2OS - COP4600");

MODULE_VERSION("0.1");

static int major_number;

static int openingCount_DeviceOutput = 0;

static struct class *pa2OSOutputClass = NULL;

static struct device *pa2OSOutputDevice = NULL;

extern struct mutex sharedData_access_mutex;

extern struct sharedMemory *dataShared;

/**

 * Since this is just an output module, we decided to remove the write function

 * and keep the open close and read, since technically that's all we need to read

 * data from the shared memory

 */

static ssize_t read(struct file *, char *, size_t, loff_t *);

static int open(struct inode *, struct file *);

static int close(struct inode *, struct file *);

static struct file_operations fops =

    {

        .owner = THIS_MODULE,

        .open = open,

        .release = close,

        .read = read,

};

/*Some of the content here was written with reference to google, and ChatGPT including how to write the prototype*/

/*We also relied on some content from the first programming assignment here*/

static int __init initialize(void)

{

    printk(KERN_INFO "lkm_pa2-out init function called -> installing the output module....\n");

    major_number = register_chrdev(0, DEVICE_NAME, &fops);

    if (major_number < 0)

    {

        printk(KERN_ALERT "lkm_pa2-out could not register number.\n");

        return major_number;
    }

    printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> Registered correctly with major number %d\n", major_number);

    pa2OSOutputClass = class_create(THIS_MODULE, CLASS_NAME);

    if (IS_ERR(pa2OSOutputClass))

    {

        unregister_chrdev(major_number, DEVICE_NAME);

        printk(KERN_ALERT "Device class failed to be registered (failed...).\n");

        return PTR_ERR(pa2OSOutputClass);
    }

    printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> Device class registered correctly.\n");

    pa2OSOutputDevice = device_create(pa2OSOutputClass, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);

    if (IS_ERR(pa2OSOutputDevice))

    {

        class_destroy(pa2OSOutputClass);

        unregister_chrdev(major_number, DEVICE_NAME);

        printk(KERN_ALERT "lkm_pa2-out (pa2OS_outModule) -> Device was not created successfully.... Failed to create thed device.\n");

        return PTR_ERR(pa2OSOutputDevice);
    }

    printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> Device was created successfully....\n");

    return 0;
}

static int open(struct inode *inodep, struct file *filep)

{

    if (openingCount_DeviceOutput > 0)

    {

        printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> Device is already open.\n");

        return -EBUSY;
    }

    openingCount_DeviceOutput++;

    try_module_get(THIS_MODULE);

    printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> opened now.\n");

    return 0;
}

static void __exit destroy(void)

{

    printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> removing module.\n");

    device_destroy(pa2OSOutputClass, MKDEV(major_number, 0));

    class_unregister(pa2OSOutputClass);

    class_destroy(pa2OSOutputClass);

    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> Goodbye from the LKM!\n");

    unregister_chrdev(major_number, DEVICE_NAME);

    return;
}

static int close(struct inode *inodep, struct file *filep)

{

    if (openingCount_DeviceOutput <= 0)

    {

        printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> Device is already closed,we will not close it again.\n");

        return -EBUSY;
    }

    printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> output device closed.\n");

    openingCount_DeviceOutput--;

    module_put(THIS_MODULE);

    return 0;
}

/*The linux Kernel documentation was referenced, along with other resources such as Google, and chatGPT*/

static ssize_t read(struct file *filep, char *buffer, size_t len, loff_t *offset)

{

    if (*offset >= MAXLENGTH)

        return 0;

    if (dataShared == NULL || dataShared->length == 0)

    {

        printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> The shared memory doesn't contain input, skipping reading output for now.....\n");

        return -EFAULT;
    }

    // print that we are waiting for the lock
    printk(KERN_ALERT "lkm_pa2-out (pa2OS_outModule) -> waiting for the lock to read the content of the shared data space.....\n");

    if (!mutex_trylock(&sharedData_access_mutex))

    {

        printk(KERN_ALERT "lkm_pa2-out (pa2OS_outModule) -> try lock failed, we will wait for the lock, skipping reading output for now till the shared memory is free to be accessed.....\n");

        return -EBUSY;
    }

    // lock acquired

    printk(KERN_ALERT "lkm_pa2-out (pa2OS_outModule) -> Lock acquired, in the critical section now.....\n");

    if (copy_to_user(buffer, dataShared->data + *offset, dataShared->length) == 0)
    {

        printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> Read %d bytes from the shared data space.\n", len);

        *offset += len;

        // after we print we simply free the input shared data since technically we will not reuse it once it is

        // printed

        memset(dataShared, 0, sizeof(struct sharedMemory));

        memset(dataShared->data, 0, MAXLENGTH);

        dataShared->length = 0;

        // affirm that the lock was released

        printk(KERN_ALERT "lkm_pa2-out (pa2OS_outModule) -> Lock released, out of the critical section now and data was printed.....\n");

        mutex_unlock(&sharedData_access_mutex);

        return len;
    }
    else
    {

        printk(KERN_INFO "lkm_pa2-out (pa2OS_outModule) -> Failed to read %d bytes from the shared data space.\n", len);

        // affirm that the lock was released

        memset(dataShared, 0, sizeof(struct sharedMemory));

        memset(dataShared->data, 0, MAXLENGTH);

        dataShared->length = 0;

        printk(KERN_ALERT "lkm_pa2-out (pa2OS_outModule) -> Lock released, out of the critical section now.....\n");

        mutex_unlock(&sharedData_access_mutex);

        return -EFAULT;
    }
}

module_init(initialize);

module_exit(destroy);