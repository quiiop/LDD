#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "main.h"
#include "fops.h"

static int async_notify_major = 0, async_notify_minor = 0;
static struct async_notify_dev *async_notify_dev = NULL;

MODULE_AUTHOR("KUO");
MODULE_LICENSE("GPL");

/*
driver裡的struct file_operation fasync是用來設定async的，
file_operation fasync是透過在user space user call fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | FASYNC),
F_SETFL最終會call setftl 裡面就有call filp->f_op->fasync()
*/
static struct file_operations fops = {
	.open  = async_notify_open,
	.read  = async_notify_read,
	.write = async_notify_write,
	.fasync = async_notify_fasync,
	.release = async_notify_release,
};

/*
struct async_notify_dev {
	struct cdev cdev;
	struct mutex mutex;
	struct fasync_struct *async_queue;
	loff_t buf_len;
	char buff[BUFF_SIZE];
};
*/

static int __init m_init(void)
{
    dev_t devno;
    int err = 0;
    
    async_notify_dev = kmalloc(sizeof(struct async_notify_dev), GFP_KERNEL);
    memset(async_notify_dev, 0, sizeof(struct async_notify_dev));

    /*1. 申請設備號, MODULE_NAME 會決定你在/dev/看到的device file name*/
    err = alloc_chrdev_region(&devno, async_notify_minor, ASYNC_NOTIFY_DEV_NR, MODULE_NAME);
    if (err < 0) {
		printk(KERN_WARING "Can't get major!\n");
		goto on_error;
	}
    async_notify_major = MAJOR(devno);

    /*2. 初始化cdev*/
    cdev_init(&async_notify_dev->cdev, &fops);
    devno = MKDEV(async_notify_major, async_notify_minor);
    err = cdev_add(&async_notify_dev->cdev, devno, ASYNC_NOTIFY_DEV_NR);
    if (err) {
		printk(KERN_WARING "Error when adding ioctl dev");
		goto on_error;
	}

    /*3. 初始化mutex*/
    mutex_init(&async_notify_dev->mutex);

    /*初始化async_queue這件事，等到driver fasync被call*/
    return 0;

on_error:
	kfree(async_notify_dev);
	return err;
}

static void __exit m_exit(void)
{

}

module_init(m_init);
moduel_exit(m_exit);