#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "main.h"
#include "fops.h"

int async_notify_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "%s() is invoked\n", __FUNCTION__);

	filp->private_data = container_of(inode->i_cdev, struct async_notify_dev, cdev);

	return 0;
}

ssize_t async_notify_read(struct file *filp, char __user *buff, size_t count,
			  loff_t *f_pos)
{
    int retval;
    struct async_notify_dev *dev = filp->private_data;

    printk(KERN_INFO "%s() is invoked\n", __FUNCTION__);

    if (mutex_lock_interruptible(&dev->mutex)){
        return -ERESTARTSYS;
    }

    if (count > dev->buf_len - *f_pos){
        count = dev->buf_len - *f_pos;
    }

    if (copy_to_user(buff, dev->buff + *f_pos, count)){
        retval = -EFAULT;
        goto cpy_user_error;
    }

    *f_pos += count;
    retval = count;

cpy_user_error:
    mutex_unlock(&dev->mutex);
    return count;
}