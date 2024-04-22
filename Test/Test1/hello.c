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
#include <asm/uaccess.h>         /* copy_to copy_from _user */

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Leonardo Suriano <leonardo.suriano@live.it>");
MODULE_DESCRIPTION("example of using read");
MODULE_VERSION("1.0");

#define DEVICE_MAX_SIZE (20)
int hello_major = 0;
int hello_minor = 0;
unsigned int hello_nr_devs = 1;
int device_max_size = DEVICE_MAX_SIZE;

struct hello_dev {
	char *p_data;              /* pointer to the memory allocated */
	struct cdev cdev;	         /* Char device structure		*/
};
struct hello_dev *hello_devices;	/* allocated in hello_init_module */

static struct class *scull_class;
static dev_t _dev_no;

/*
 * Open and close
 */

int hello_open(struct inode *inode, struct file *filp)
{
    struct hello_dev *dev; /* device information */
    printk(KERN_INFO "[LEO] performing 'open' operation\n");
   
    dev = container_of(inode->i_cdev, struct hello_dev, cdev);
    if(!dev){
        printk(KERN_INFO "[LEO] Open fail\n");
    }else{
        printk(KERN_INFO "[LEO] Open successful\n");
        printk(KERN_INFO "[LEO] %s\n", dev->p_data);
    }

    filp->private_data = dev; /* for other methods */
	return 0;          /* success */
}

/*
 * Create a set of file operations for our hello files.
 * All the functions do nothig
 */

struct file_operations hello_fops = {
    .owner =    THIS_MODULE,
    .open =     hello_open,
};

/*
 * Set up the char_dev structure for this device.
 */
static void hello_setup_cdev(struct hello_dev *dev)
{
	int err, devno = MKDEV(hello_major, hello_minor);

	cdev_init(&dev->cdev, &hello_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &hello_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_WARNING "[LEO] Error %d adding hello cdev_add", err);

  printk(KERN_INFO "[LEO] cdev initialized\n");
}

void hello_cleanup_module(void)
{
	dev_t devno = MKDEV(hello_major, hello_minor);
    cdev_del(&(hello_devices->cdev)); // 移除cdev, 從內核註銷cdev
    /* freeing the memory */
    if((hello_devices -> p_data) != 0){
        kfree(hello_devices -> p_data);
        printk(KERN_INFO "[LEO] kfree the string-memory\n");
    }
    if((hello_devices) != 0){
        kfree(hello_devices);
        printk(KERN_INFO "[LEO] kfree hello_devices\n");
    }
	unregister_chrdev_region(devno, hello_nr_devs); // 註銷cdev<major, minor> number
	printk(KERN_INFO "[LEO] cdev deleted, kfree, chdev unregistered\n");

    // Remove the device file
    device_destroy(scull_class, _dev_no); // 記得檢查/dev/hello是否消失
    // Remove the device class
    class_destroy(scull_class); // 記得檢查/sys/class/hello是否消是
}

/*
 * The init function is used to register the chdeiv allocating dinamically a
 * new major number (if not specified at load/compilation-time)
 */

static int hello_init(void)
{
	int result =0;
	dev_t dev = 0;

	if (hello_major) {
		printk(KERN_INFO "[LEO] static allocation of major number (%d)\n",hello_major);
		dev = MKDEV(hello_major, hello_minor);
		result = register_chrdev_region(dev, hello_nr_devs, "hello");
	} else {
		printk(KERN_INFO "[LEO] dinamic allocation of major number\n");
		result = alloc_chrdev_region(&dev, hello_minor, hello_nr_devs, "hello");
		hello_major = MAJOR(dev);
	}
    _dev_no = dev;

	if (result < 0) {
		printk(KERN_WARNING "[LEO] hello: can't get major %d\n", hello_major);
		return result;
    }

    hello_devices = kmalloc(hello_nr_devs * sizeof(struct hello_dev), GFP_KERNEL);
    if (!hello_devices) {
        result = -ENOMEM;
        printk(KERN_WARNING "[LEO] ERROR kmalloc dev struct\n");
        goto fail;  /* Make this more graceful */
    }

    memset(hello_devices, 0, hello_nr_devs * sizeof(struct hello_dev));
    /* Initialize the device. */

    hello_devices -> p_data = (char*)kmalloc(device_max_size * sizeof(char), GFP_KERNEL);
    hello_devices->p_data = "hello kuo";
    if (!hello_devices -> p_data) {
        result = -ENOMEM;
        printk(KERN_WARNING "[LEO] ERROR kmalloc p_data\n");
        goto fail;  /* Make this more graceful */
    }
    hello_setup_cdev(hello_devices);

    // create device file
    scull_class = class_create("hello"); // /sys/class/hello
    if (IS_ERR(scull_class)){
        printk(KERN_WARNING "Failed to create scull class\n");
        return PTR_ERR(scull_class);
    }
    device_create(scull_class, NULL, dev, NULL, "hello"); // /dev/hello

    return 0;

    fail:
    hello_cleanup_module();
    return result;
}

/*
 * The exit function is simply calls the cleanup
 */

static void hello_exit(void)
{
	  printk(KERN_INFO "[LEO] --calling cleanup function--\n");
	  hello_cleanup_module();
}

module_init(hello_init);
module_exit(hello_exit);