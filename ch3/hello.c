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
#include "hello.h"

/*
1. 申請(major, minor) number
2. 初始化 struc hello_dev
3. 初始化 struct cdev
    3.1 cdev_init 
        1. 初始化kobject
        2. 把cdev和file_operations相連
    3.2 cdev_add (將struct cdev載入系統)
        1. 把struct cdev和dev_t dev連接
        2. call kobj_map, 把struct cdev載入到static struct kobj_map *cdev_map, cdev_map用以記載現在系統擁有的struct cdev 
4. 定義driver的file operation
5. 移除module時, 釋放空間
    5.1 釋放struct cdev , 用cdev_del( )
    5.2 釋放struct hello_dev , 用kfree( )
    5.3 釋放(major, minor) number , 用unregister_chrdev_region（ ）
*/

MODULE_LICENSE("GPL");

int hello_major = 0;
int hello_minor = 0;
unsigned int hello_nr_devs = 1; //指定minor申請多少個
int device_max_size = DEVICE_MAX_SIZE;

/*
struct hello_dev {
    char *p_data;
    struct cdev cdev;
}; 
*/
struct hello_dev *hello_devices;

/*4. 定義driver的file operation*/
int hello_open(struct inode *inode, 
                struct file *filp)
{
    struct hello_dev *dev;
    dev = container_of(inode->i_cdev, struct hello_dev, cdev);
    if (!dev){
        printk(KERN_WARNING "[KUO] Open failed\n");
    }else{
        printk(KERN_INFO "[KUO] Open successful\n");
    }

    filp->private_data = dev;
    return 0;
}   

ssize_t hello_read(struct file *filp, 
                    char __user *buff, 
                    size_t count, 
                    loff_t *f_ops)
{
    int retval = 0;
    
    if (count > device_max_size){
        printk(KERN_WARNING "[LEO] hello: trying to read more than possible. Aborting read\n");
        retval = -EFBIG;
    }
    
    int n = copy_to_user(buff, (void *)hello_devices->p_data, count); 
    printk(KERN_INFO "[KUO] READ : copy %d char\n", n);

    return retval;
}

int hello_release(struct inode *inode,
                    struct file *filp)
{
    printk(KERN_INFO "[KUO] Close file\n");
    return 0;
}

struct file_operations hello_fops = {
     .open = hello_open,
     .release = hello_release,
};

void hello_cleanup_module(void)
{
    dev_t devno = MKDEV(hello_major, hello_minor);
    
    /*釋放struct cdev*/
    cdev_del(&(hello_devices->cdev));
    
    /*釋放struct hello_dev*/
    if((hello_devices -> p_data) != 0){
        kfree(hello_devices -> p_data);
        printk(KERN_INFO "[KUO] kfree the string-memory\n");
    }
    if((hello_devices) != 0){
        kfree(hello_devices);
        printk(KERN_INFO "[KUO] kfree hello_devices\n");
    }

    /*釋放(major, minor) number*/
    unregister_chrdev_region(devno, hello_nr_devs);
    printk(KERN_INFO "[KUO] cdev deleted, kfree, chdev unregistered\n");
}

static void hello_setup_cdev(struct hello_dev *dev)
{
    int err, devno = MKDEV(hello_major, hello_minor);
    
    cdev_init(&dev->cdev, &hello_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &hello_fops; // 這段可以不用寫
    
    err = cdev_add(&dev->cdev, devno, 1);
    if (err){
        printk(KERN_WARNING "[KUO] Error %d adding hello cdev_add", err);
    }

    printk(KERN_INFO "[KUO] cdev initialized\n");
}

static __init int hello_init(void)
{
    int result = 0;
    dev_t dev = 0;

    /*1. 申請(major, minor) number*/
    printk(KERN_INFO "[KUO] dinamic allocation of major number\n");
    result = alloc_chrdev_region(&dev, hello_minor, hello_nr_devs, "hello");
    hello_major = MAJOR(dev);
    
    if (result < 0){
        printk(KERN_WARNING "[KUO] hello: can't get major %d\n", hello_major);
        return result;
    }else{
        printk(KERN_INFO "[KUO] hello: success get major %d\n", hello_major);
    }

    /*2. 初始化struc hello_dev, struct cdev*/
    /*如果hello_nr_devs大於1, hello_devices就變成一個Array*/
    hello_devices = kmalloc(hello_nr_devs*sizeof(struct hello_dev), GFP_KERNEL);
    if (!hello_devices) {
        result = -ENOMEM;
        printk(KERN_WARNING "[KUO] hello_devices apply failed\n");
    }
    memset(hello_devices, 0, hello_nr_devs*sizeof(struct hello_dev));

    hello_devices->p_data = (char *)kmalloc(device_max_size*sizeof(char), GFP_KERNEL);
    if (!hello_devices->p_data){
        result = -ENOMEM;
        printk(KERN_WARNING "[KUO] hello_devices->p_data apply failed\n");
    }
    hello_devices->p_data = "Hello KUO";

    /*3. 初始化 struct cdev*/
    hello_setup_cdev(hello_devices);

    return result;
}

static __exit void hello_exit(void)
{
    printk(KERN_INFO "[KUO] --calling cleanup function--\n");
	hello_cleanup_module();
}

module_init(hello_init);
module_exit(hello_exit);