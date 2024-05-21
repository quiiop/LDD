#include <linux/init.h>          /* needed for module_init and exit */
#include <linux/module.h>
#include <linux/moduleparam.h>   /* needed for module_param */
#include <linux/kernel.h>        /* needed for printk */
#include <linux/sched.h>         /* needed for current-> */
#include <linux/types.h>         /* needed for dev_t type */
#include <linux/kdev_t.h>        /* needed for macros MAJOR, MINOR, MKDEV... */
#include <linux/fs.h>            /* needed for register_chrdev_region, file_operations */
#include <linux/slab.h>		       /* kmalloc(),kfree() */
#include <linux/uaccess.h>         /* copy_to copy_from _user */

#include "ioctl_01.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leonardo Suriano <leonardo.suriano@live.it>");
MODULE_DESCRIPTION("example of using ioctl(...)");
MODULE_VERSION("1.0");

int ioctl_01_major = 0;
int ioctl_01_minor = 0;
unsigned int ioctl_01_nr_devs = 1;
int device_max_size = DEVICE_MAX_SIZE;

unsigned long long read_times = 0;
unsigned long long write_times = 0;
unsigned char buffer_in_use = 0;

struct ioctl_01_dev *ioctl_01_devices;

int my_open(struct inode *inode, struct file *filp)
{
    struct ioctl_01_dev *my_dev;
    printk(KERN_INFO "performing 'open' operation\n");
    my_dev = container_of(inode->i_cdev, struct ioctl_01_dev, cdev);
    filp->private_data = my_dev;
    
    return 0;
}

ssize_t my_read(struct file *filp, char __user *buf, size_t cnt, loff_t *f_ops)
{
    struct ioctl_01_dev *my_dev = filp->private_data;
    ssize_t retval = 0;
    char *p_data_temp;

    /*read的資料大小不能超過buffer大小*/
    if (cnt > device_max_size){
        printk(KERN_WARNING "[LEO] ioctl_01: trying to read more than possible. Aborting read\n");
        retval = -EFBIG;
        goto out;
    }

    if (down_interruptible(&(my_dev->sem_ioctl_01))){
        printk(KERN_WARNING "[LEO] ioctl_01: Device was busy. Operation aborted\n");
        return -ERESTARTSYS;
    }

    switch(buffer_in_use){
       case FIRST_BUFFER:
        p_data_temp=my_dev->p_data_01;
        break;
       case SECOND_BUFFER:
        p_data_temp=my_dev->p_data_02;
        break;
       default:
        printk(KERN_WARNING "[LEO] ioctl_01: no valid buffer in use. \n");
        /*即便發生錯誤也要先release semaphore*/
        up(&(my_dev->sem_ioctl_01));
        return -EAGAIN; //to find the right value to return
    }

    /*copy_to_user返回尚未寫進buffer的字元，所以正常是會return 0，表示全部資料都寫入buffer了*/
    if (copy_to_user(buf, (void*)p_data_temp, cnt)) {
        printk(KERN_WARNING "[LEO] ioctl_01: can't use copy_to_user. \n");
 		retval = -EPERM;
 		goto out_and_Vsem;
 	}

    read_times++;
    up(&(my_dev->sem_ioctl_01));
    return retval;

    out_and_Vsem:
        up(&(my_dev->sem_ioctl_01));
        return retval;

    out:
        return retval;
}

ssize_t my_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *f_ops)
{
    struct ioctl_01_dev *my_dev = filp->private_data;
    int retval = 0;
    char *p_data_temp;

    if (cnt > device_max_size){
        printk(KERN_WARNING "[LEO] ioctl_01: trying to write more than possible. Aborting write\n");
        retval = -EFBIG;
        goto out;
    }

    if (down_interruptible(&(my_dev->sem_ioctl_01))){
        printk(KERN_WARNING "[LEO] ioctl_01: Device was busy. Operation aborted\n");
        return -ERESTARTSYS;
    }

    switch(buffer_in_use){
       case FIRST_BUFFER:
        p_data_temp=ioctl_01_devices -> p_data_01;
        break;
       case SECOND_BUFFER:
        p_data_temp=ioctl_01_devices -> p_data_02;
        break;
       default:
        printk(KERN_WARNING "[LEO] ioctl_01: no valid buffer in use. \n");
        up(&(ioctl_01_devices->sem_ioctl_01));
        return -EAGAIN; //to find the right value to return
    }

    printk(KERN_INFO "[KUO] Start write buffer_%d\n", buffer_in_use);
    if (copy_from_user((void*)p_data_temp, buf, cnt)) {
        printk(KERN_WARNING "[LEO] ioctl_01: can't use copy_from_user. \n");
        retval = -EPERM;
        goto out_and_Vsem;
    }
    printk(KERN_INFO "[KUO] buffer_%d content = %s\n", buffer_in_use, p_data_temp);

    write_times++;
    up(&(my_dev->sem_ioctl_01));
    return retval;

    out_and_Vsem:
        up(&(my_dev->sem_ioctl_01));
        return retval;
    out:
        return retval;
}

long my_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct ioctl_01_dev *my_dev = filp->private_data;
    int err = 0;
    int retval = 0;

    /*TYPE用於辨識這個ioctl屬於哪個driver，type field是8 bits，所以一個kernel最多可以有256個ioctl type。*/
    if (_IOC_TYPE(cmd) != IOCTL_01_IOC_MAGIC){
        printk(KERN_WARNING "[KUO] cmd err. \n");
        return -ENOTTY;
    }

    /*NR(number)一個driver可以有多個ioctl command，nr field是是8 bits，所以一個driver可以有256 ioctl command。*/
    /*IOCTL_01_IOC_MAXNR是目前定義ioctl command的數量*/
    if (_IOC_NR(cmd) > IOCTL_01_IOC_MAXER){
        printk(KERN_WARNING "[KUO] ioctl_01: _IOC_NR(cmd) > IOCTL_01_IOC_MAXNR => false. Aborting ioctl\n");
        return -ENOTTY;
    }

    /*  _IOC_WRITE = 1U
        _IOC_READ = 2U
        這裡要注意user的read command對於driver是要執行write的動作。*/
    /*_IO_DIR，dir是指direction，表示read or write*/
    if (_IOC_DIR(cmd) & _IOC_READ){
        err = !access_ok((const void __user *)arg, _IOC_SIZE(cmd));
    }else if(_IOC_DIR(cmd) & _IOC_WRITE)
    {
        err = !access_ok((const void __user *)arg, _IOC_SIZE(cmd));
    }
    if (err){
        return -EFAULT;
    }

    switch (cmd)
    {
    case DEVICE_IOCRESET:
        printk(KERN_INFO "DEVICE_IOCRESET\n");
        if (down_interruptible(&(my_dev->sem_ioctl_01))){
            printk(KERN_WARNING "[LEO] ioctl_01: Device was busy. Operation aborted\n");
            return -ERESTARTSYS;
        }
        buffer_in_use = 0; /* Buffers cannot be used */
        up(&(my_dev->sem_ioctl_01));
        break;
    
    case SET_FIRST_BUFFER:
        printk(KERN_INFO "SET_FIRST_BUFFER\n");
        if (down_interruptible(&(my_dev->sem_ioctl_01))){
            printk(KERN_WARNING "[LEO] ioctl_01: Device was busy. Operation aborted\n");
            return -ERESTARTSYS;
        }
        buffer_in_use = FIRST_BUFFER;
        up(&(my_dev->sem_ioctl_01));
        break;

    case SET_SECOND_BIFFER:
        printk(KERN_INFO "SET_SECOND_BIFFER\n");
        if (down_interruptible(&(my_dev->sem_ioctl_01))){
            printk(KERN_WARNING "[LEO] ioctl_01: Device was busy. Operation aborted\n");
            return -ERESTARTSYS;
        }
        buffer_in_use = SECOND_BUFFER;
        up(&(my_dev->sem_ioctl_01));
        break;

    case WHICH_BUFFER_USE:
        printk(KERN_INFO "[KUO] WHICH_BUFFER_USE\n");
        printk(KERN_INFO "[KUO] buffer_in_use %d\n", buffer_in_use);
        retval = __put_user(buffer_in_use, (int __user*)arg);
        printk(KERN_INFO "[KUO] user provide user buffer addr %p\n", ((int __user*)arg));
        // return buffer_in_use;
        break;

    case SET_WHICH_BUFFER_USE:
        printk(KERN_INFO "[KUO] SET_WHICH_BUFFER\n");
        if (down_interruptible(&(my_dev->sem_ioctl_01))){
            printk(KERN_WARNING "[LEO] ioctl_01: Device was busy. Operation aborted\n");
            return -ERESTARTSYS;
        }
        retval = __get_user(buffer_in_use, (int __user*)arg);
        printk(KERN_INFO "[KUO] buffer_in_use %d\n", buffer_in_use);
        
        if (buffer_in_use >2 || buffer_in_use<=0){
            up(&(my_dev->sem_ioctl_01));
            return -ENOTTY;
        }
        up(&(my_dev->sem_ioctl_01));
        break;
        
    default:
        return -ENOTTY;
    }
    return retval;
}

int my_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "performing 'close' operation\n");
    return 0;
}

struct file_operations my_fops = {
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
    .unlocked_ioctl = my_unlocked_ioctl,
};

void my_cleanup(void)
{
    /*註銷cdev*/
    cdev_del(&(ioctl_01_devices->cdev));

    /*註銷device*/
    if((ioctl_01_devices->p_data_01) != 0){
        kfree(ioctl_01_devices->p_data_01);
        printk(KERN_WARNING " kfree the string-memory\n");
    }
    if((ioctl_01_devices) != 0){
        kfree(ioctl_01_devices);
        printk(KERN_WARNING " kfree ioctl_01_devices\n");
    }

    /*註銷設備號*/
    dev_t devno = MKDEV(ioctl_01_major, ioctl_01_minor);
    unregister_chrdev_region(devno, ioctl_01_nr_devs);
    printk(KERN_INFO "cdev deleted, kfree, chdev unregistered\n");
}

void my_setup_cdev(struct ioctl_01_dev *my_dev)
{
    int err, devno = MKDEV(ioctl_01_major, ioctl_01_minor);
    cdev_init(&my_dev->cdev, &my_fops);
    my_dev->cdev.ops = &my_fops;
    err = cdev_add (&my_dev->cdev, devno, 1); // 1是cdev count

    if (err){
        printk(KERN_WARNING "[LEO] Error %d adding ioctl_01 cdev_add", err);
    }

    printk(KERN_INFO "[LEO] cdev initialized\n");
}

static int my_init(void)
{
    int result =0;
    dev_t dev = 0;

    /*申請設備號*/
    printk(KERN_INFO "ioctl_01: dinamic allocation of major number\n");
 	result = alloc_chrdev_region(&dev, ioctl_01_minor, ioctl_01_nr_devs, "ioctl_01");
 	ioctl_01_major = MAJOR(dev);
    printk(KERN_INFO "[KUO] major number = %d\n", ioctl_01_major);

    /*建立device*/
    ioctl_01_devices = kmalloc(ioctl_01_nr_devs * sizeof(struct ioctl_01_dev), GFP_KERNEL);
    if (!ioctl_01_devices) {
        result = -ENOMEM;
        printk(KERN_WARNING "[LEO] ioctl_01: ERROR kmalloc dev struct\n");
        goto fail;  /* Make this more graceful */
    }

    memset(ioctl_01_devices, 0, ioctl_01_nr_devs * sizeof(struct ioctl_01_dev));

    /*初始化device buffer*/
    ioctl_01_devices->p_data_01 = (char *)kmalloc(device_max_size * sizeof(char), GFP_KERNEL);
    if (!ioctl_01_devices->p_data_01) {
        result = -ENOMEM;
        printk(KERN_WARNING "[LEO] ioctl_01: ERROR kmalloc p_data_01\n");
        goto fail;  /* Make this more graceful */
    }

    ioctl_01_devices->p_data_02 = (char*)kmalloc(device_max_size * sizeof(char), GFP_KERNEL);
    if (!ioctl_01_devices->p_data_02) {
        result = -ENOMEM;
        printk(KERN_WARNING "[LEO] ioctl_01: ERROR kmalloc p_data_02\n");
        goto fail;  /* Make this more graceful */
    }

    /*初始化device semaphore*/
    sema_init(&(ioctl_01_devices->sem_ioctl_01), 1);
    
    /*初始化cdev用semaphore lock*/
    if (down_interruptible(&(ioctl_01_devices->sem_ioctl_01))){
        printk(KERN_WARNING "[LEO] ioctl_01: Device was busy. Operation aborted\n");
        return -ERESTARTSYS;
    }
    //初始化device cdev
    my_setup_cdev(ioctl_01_devices);
    //set default buffer to use 
    buffer_in_use = FIRST_BUFFER;
    up(&(ioctl_01_devices->sem_ioctl_01));

    return 0;

    fail:
        my_cleanup();
        return result;
}

static void my_exit(void)
{
    printk(KERN_INFO " --calling cleanup function--\n");
 	my_cleanup();
}

module_init(my_init);
module_exit(my_exit);





















