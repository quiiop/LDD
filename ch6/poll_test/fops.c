#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "main.h"
#include "fops.h"

int my_open(struct inode *inode, struct file *filp)
{
	printk(KERN_INFO "[KUO] %s() is invoked\n", __FUNCTION__);

	filp->private_data = container_of(inode->i_cdev, struct poll_dev, cdev);

	return 0;
}

ssize_t my_read(struct file *filp, char __user *buff, size_t count,
			  loff_t *f_pos)
{
	int retval;
	struct poll_dev *dev = filp->private_data;

	pr_debug("%s() is invoked\n", __FUNCTION__);

	if (mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;

	/*dev->buf_len表示buffer現有有的資料數量*/
	/*dev->buf_len - *f_pos 等於buffer剩餘可讀的大小*/
	if (count > dev->buf_len - *f_pos)
		count = dev->buf_len - *f_pos;
	
	if (copy_to_user(buff, dev->buff + *f_pos, count)) {
		retval = -EFAULT;
		goto cpy_user_error;
	}

	*f_pos += count;
	retval = count;
	mutex_unlock(&dev->mutex);
	return retval;

cpy_user_error:
	mutex_unlock(&dev->mutex);
	return retval;
}

ssize_t my_write(struct file *filp, const char __user *buff,
			   size_t count, loff_t *f_pos)
{
	int retval;
	struct poll_dev *dev = filp->private_data;

	pr_debug("%s() is invoked\n", __FUNCTION__);

	if (mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;

	if (count > BUFF_SIZE - *f_pos)
		count = BUFF_SIZE - *f_pos;

	if (copy_from_user(dev->buff + *f_pos, buff, count)) {
		retval = -EFAULT;
		goto cpy_user_error;
	}

	/*dev->buf_len表示buffer現有有的資料數量*/
	*f_pos += count;
	dev->buf_len = *f_pos;
	retval = count;
	mutex_unlock(&dev->mutex);
	return retval;

cpy_user_error:
	mutex_unlock(&dev->mutex);
	return retval;
}

unsigned int my_poll(struct file *filp, poll_table *wait)
{
	struct poll_dev *dev = filp->private_data;
	unsigned int mask = 0;

	printk(KERN_INFO "%s() is invoked\n", __FUNCTION__);

	mutex_lock(&dev->mutex);

	/*poll_wait()只會先讓process加入wait queue，但是不會將process enter block*/
	poll_wait(filp, &dev->inq, wait);
	poll_wait(filp, &dev->outq, wait);

	/*atomic_dec_and_test()是遞減，將dev->can_rd遞減一，如果遞減後等於0，則return true，反之return false。*/
	/*在timer_func裡都是用atomic_set()的方式設置為1，表示I/O可以用，就是為了配合my_poll的atomic_dec_and_test()*/
	/*所以atomic_dec_and_test() return true，才能enter if設定mask，my_poll最後在return mask，user就可以透過mask得知IO的情況*/
	if (atomic_dec_and_test(&dev->can_rd)) {
		printk(KERN_INFO "Now fd can be read\n");
		mask |= POLLIN | POLLRDNORM;
	}

	if (atomic_dec_and_test(&dev->can_wr)) {
		printk(KERN_INFO "Now fd can be written\n");
		mask |= (POLLOUT | POLLWRNORM);
	}

	mutex_unlock(&dev->mutex);

	printk(KERN_INFO "return mask = 0x%x\n", mask);
	return mask;
}