#include <linux/init.h>          /* needed for module_init and exit */
#include <linux/module.h>
#include <linux/moduleparam.h>   /* needed for module_param */
#include <linux/kernel.h>        /* needed for printk */
#include <linux/sched.h>         /* needed for current-> */
#include <linux/types.h>         /* needed for dev_t type */
#include <linux/kdev_t.h>        /* needed for macros MAJOR, MINOR, MKDEV... */
#include <linux/fs.h>            /* needed for register_chrdev_region, file_operations */
#include <linux/cdev.h>          /* cdev definition */
#include <linux/slab.h>		       /* kmalloc(),kfree() */
#include <linux/uaccess.h>         /* copy_to copy_from _user */
#include <linux/semaphore.h>
#include <linux/cdev.h>

#include "hello.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leonardo Suriano <leonardo.suriano@live.it>");
MODULE_DESCRIPTION("example of using read");
MODULE_VERSION("1.0");

int hello_major = 0;
int hello_minor = 0;
unsigned int hello_nr_devs = 1; // 申請多少minor數量
int device_max_size = DEVICE_MAX_SIZE;

struct hello_dev *hello_devices;

int hello_open(struct inode *inode, struct file *filp)
{
    /*
    hello_open主要的任務是從inode->-i_cdev中透過container_of取得我們的hello_devices
    記得要把struct file和你的結構相連
    */

    struct hello_dev *dev;
    printk(KERN_INFO "[KUO] performing 'open' operation\n");
    dev = container_of(inode->i_cdev, struct hello_dev, cdev);
    
    filp->private_data = dev;
    
    return 0;
}

int hello_release(struct inode *inode, struct file *filp)
{
    /*這裡之後可以改成user space關閉device file可以釋放一些資源*/
    printk(KERN_INFO "[KUO] performing 'release' operation\n");
    return 0;
}

/*把hello_devices->p_data資料傳到user space*/
ssize_t hello_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;
    if (count > device_max_size){ // 這是讀hello_devices->p_data
        printk(KERN_WARNING "[KUO] hello: trying to read more than possible. Aborting read\n");
        retval = -EFBIG;
        goto out;
    } 

    /*
    hello_devices只有一個, 所以正式讀資料前, 需要用semaphore管理, 可以用down_interruptible check
    down_interruptible return 0表示semaphore >0 , 其他則表示semaphore <0
    */
    if (down_interruptible(&hello_devices->sem_hello)){
        printk(KERN_WARNING "[KUO] hello: Device was busy. Operation aborted\n");
        return -ERESTARTSYS;
    }

    /*
    copy_to_user 函数返回未能被成功复制的字节数。如果返回值为 0，则表示全部数据都成功复制到用户空间
    */
    // printk(KERN_INFO "[KUO] READ DATA %s\n", hello_devices->p_data);
    if (copy_to_user(buf, (void *)hello_devices->p_data, count)){
        printk(KERN_WARNING "[KUO] hello: can't use copy_to_user. \n");
        retval = -EPERM;
        goto out_and_Vsem;
    }

    // 成功复制数据后立即释放信号量 
    // up(&(hello_devices->sem_hello));
    return retval; // 返回值为 0 表示成功

    out_and_Vsem: // 即便失敗也要釋放semaphore
        up(&(hello_devices->sem_hello));
        return retval;
    out:
        return retval;
}

/*把user space資料傳道hello_devices->p_data*/
ssize_t hello_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    up(&(hello_devices->sem_hello));
    /*int retval = 0;
    if (count > device_max_size){
        printk(KERN_WARNING "[KUO] hello: trying to write more than possible. Aborting write\n");
        retval = -EFBIG;
        goto out;
    }

    if (down_interruptible(&hello_devices->sem_hello)){
        printk(KERN_WARNING "[KUO] hello: Device was busy. Operation aborted\n");
        return -ERESTARTSYS;
    }

    if (copy_from_user((void *)hello_devices->p_data, buf, count)){
        printk(KERN_WARNING "[LEO] hello: can't use copy_from_user. \n");
        retval = -EPERM;
        goto out_and_Vsem;
    }

    printk(KERN_INFO "[KUO] successful write info %s\n", hello_devices->p_data);
    up(&(hello_devices->sem_hello));
    return retval; 

    out_and_Vsem:
        up(&(hello_devices->sem_hello));
        return retval;
    out:
        return retval;
    */
   return 0;
}

struct file_operations hello_fops = {
    .read       = hello_read,
    .write      = hello_write,
    .open       = hello_open,
    .release    = hello_release,
};

static void hello_cleanup_module(void)
{
    /*
    hello_cleanup_module 注意順序 不要先註銷設備號
    1. 釋放struct cdev
    2. 釋放struct hello_dev
    3. 註銷設備號
    */

    /*1. 釋放struct cdev*/
    cdev_del(&hello_devices->cdev);
    printk(KERN_INFO "[KUO] del struct cdev\n");

    /*2. 釋放struct hello_dev*/
    if (hello_devices->p_data){
        kfree(hello_devices->p_data);
        printk(KERN_INFO "[KUO] kfree the string-memory\n");
    }
    if (hello_devices) {
        kfree(hello_devices);
        printk(KERN_INFO "[KUO] kfree hello_devices\n");
    }

    /*3. 註銷設備號*/
    dev_t devno = MKDEV(hello_major, hello_minor);
    unregister_chrdev_region(devno, hello_nr_devs);
    printk(KERN_INFO "[KUO] cdev deleted, kfree, chdev unregistered\n");
}

static void hello_setup_cdev(struct hello_dev *dev)
{
    /*
    hello_setup_cdev要負責初始化3個東西
    1. p_data
    2. struct semaphore
    3. struct cdev
        3.1 cdev_init
        3.2 cdev_add
    */

    int result = 0;
    /*1. p_data*/
    dev->p_data = kmalloc(device_max_size*sizeof(char), GFP_KERNEL);
    if (!dev->p_data) {
        result = -ENOMEM;
        printk(KERN_WARNING "[KUO] ERROR kmalloc p_data\n");
        goto fail;
    }

    /*2. struct semaphore*/
    sema_init(&(dev->sem_hello), 1); // wait_list會自動創建

    /*3. struct cdev*/
    int err = 0;
    dev_t devno = MKDEV(hello_major, hello_minor);
    cdev_init(&dev->cdev, &hello_fops);
    err = cdev_add(&dev->cdev, devno, 1);
    if (err){
        printk(KERN_WARNING "[KUO] Error %d adding hello cdev_add", err);
    }

    printk(KERN_INFO "[KUO] cdev initialized\n");

    return ;
    fail:
        hello_cleanup_module();
}

static int hello_init(void)
{
    int result = 0;
    dev_t dev = 0;

    printk(KERN_INFO "[KUO] hello module init start\n");
    /*申請設備號*/
    result = alloc_chrdev_region(&dev, 0, hello_nr_devs, "hello");
    hello_major = MAJOR(dev);
    printk(KERN_INFO "[KUO] major number = %d\n", hello_major);

    if (result<0){
        printk(KERN_WARNING "[KUO] hello: can't get major %d\n", hello_major);
		return result;
    }

    /*初始化這個driver持有的struct*/
    hello_devices = kmalloc(hello_nr_devs * sizeof(struct hello_dev), GFP_KERNEL);
    if (!hello_devices){
        result = -ENOMEM;
        printk(KERN_WARNING "[KUO] ERROR kmalloc dev struct\n");	
        goto fail;
    }

    memset(hello_devices, 0, hello_nr_devs*sizeof(struct hello_dev));
    hello_setup_cdev(hello_devices);

    printk(KERN_INFO "[KUO] hello module init over\n");
    return 0;

    fail:
        hello_cleanup_module();
        return result;
}

static void hello_exit(void)
{
    hello_cleanup_module();
    printk(KERN_INFO "[KUO] hello module exit\n");
}

module_init(hello_init);
module_exit(hello_exit);