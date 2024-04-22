#include <linux/init.h>
#include <linux/slab.h> // define kmalloc()
#include <linux/fs.h> // define alloc_chrdev_region()
#include <linux/moduleparam.h>
#include <linux/module.h> // define macro module_init() module_exit()
#include <linux/string.h> // define memset()
#include <linux/uaccess.h> // define copy_to_user()
#include <linux/device.h> // create device file

#include "scull.h"
#define DEVICE_NAME "scull_device"

MODULE_LICENSE("GPL");

int scull_major;
int scull_minor;
struct scull_dev *scull_devices;
int scull_nr_devs = SCULL_NR_DEVS;
int scull_quantum = SCULL_QUANTUM;
int scull_qset =    SCULL_QSET;
static struct class *scull_class;
static dev_t dev_no;

// function prototype
ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    return 0;
}
ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    return 0;
}

int scull_open(struct inode *inode, struct file *filp)
{
    printk(KERN_NOTICE "inode major %d\n", MAJOR(inode->i_rdev));
    printk(KERN_NOTICE "inode minor %d\n", MINOR(inode->i_rdev));
    struct scull_dev *dev;
    struct cdev *p = inode->i_cdev;
    if (!p){
        printk(KERN_NOTICE "find %p\n", p);
    }else{
        printk(KERN_NOTICE "p NULL\n", p);
    }

    dev = container_of(inode->i_cdev, struct scull_dev, cdev);    
    if(!dev){
        printk(KERN_NOTICE "Open succeddful %d\n", dev->name);
    }else{
        printk(KERN_NOTICE "Open fail\n");
    }

    filp->private_data = dev;

    return 0;
}


struct file_operations scull_fops = {
    .read = scull_read,
    .write = scull_write,
    .open = scull_open,
};

static void scull_setup_cdev(struct scull_dev *dev, int index)
{
    dev->name = 100;
    int err, devno = MKDEV(scull_major, (scull_minor+index));
    cdev_init(&dev->cdev, &scull_fops); // init character device
    dev->cdev.ops = &scull_fops;
    err = cdev_add(&dev->cdev, devno, 1); // register character device in kernel
    if (err){
        printk(KERN_NOTICE "ERROR failed add scull %d\n", index);
    }else{
        printk(KERN_NOTICE "SUCCESSFUL add scull %d\n", index);
    }
    printk(KERN_NOTICE "cdev: %p, ops: %p\n", &dev->cdev, dev->cdev.ops);
}

static int __init scull_init_module(void)
{
    dev_t dev;
    int result;
    scull_minor = 0;

    // apply major&minor number
    result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
    scull_major = MAJOR(dev);
    if (result<0){
        printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
        return result;
    } else {
        printk(KERN_WARNING "scull: success get major %d\n", scull_major);
        dev_no = dev;
    }

    // apply scull device
    scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL); // scull_devices =  a pointer to Array of struct scull_dev
    if (!scull_devices){
        printk(KERN_WARNING "scull_devices fail\n");
    }
    memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

    // init scull device
    int i;
    for (i = 0; i < scull_nr_devs; i++){
        scull_devices[i].quantum = scull_quantum;
        scull_devices[i].qset = scull_qset;
        scull_setup_cdev(&scull_devices[i], i);
    }

    // create device file
    scull_class = class_create("scull_device_class"); // /sys/class/scull_device_class
    if (IS_ERR(scull_class)){
        printk(KERN_WARNING "Failed to create scull class\n");
        return PTR_ERR(scull_class);
    }

    device_create(scull_class, NULL, dev, NULL, "scull_device"); // /dev/scull_device

    return 0;
}

static void __exit scull_exit_module(void)
{
    // Remove the device file
    device_destroy(scull_class, dev_no);
    // Remove the device class
    class_destroy(scull_class);
    printk(KERN_INFO "over \n");
}

module_init(scull_init_module);
module_exit(scull_exit_module);