#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <linux/fs.h> // poll 相關的操作定義在這裡
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/timer.h> // struct timer_list
#include <linux/atomic/atomic-instrumented.h> // atomic_set

#include "main.h"
#include "fops.h"

MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

/*
struct timer_list {
	struct hlist_node	entry;
	unsigned long		expires;
	void			(*function)(struct timer_list *);
	u32			flags;

#ifdef CONFIG_LOCKDEP
	struct lockdep_map	lockdep_map;
#endif
};
*/

/*
typedef struct {
	int counter;
} atomic_t;
*/

/*
struct poll_dev {
	struct cdev cdev;
	struct mutex mutex;
	struct timer_list timer;
	unsigned long timer_counter;
	atomic_t can_wr;
	atomic_t can_rd;
	wait_queue_head_t inq;
	wait_queue_head_t outq;
	loff_t buf_len;
	char buff[BUFF_SIZE];
};
*/

static int poll_major = 0, poll_minor = 0;
static struct poll_dev *poll_dev = NULL;

static struct file_operations fops = {
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .poll = my_poll,
};

static void timer_fn(struct timer_list *t)
{
    struct poll_dev *dev = container_of(t, struct poll_dev, timer);
    printk(KERN_INFO "[KUO] call timer_func\n");

	++dev->timer_counter;

	/*
	reader每一次callback都喚醒dev->inq的reader。
	writer每兩次callback都喚醒dev->out的writer。
	*/

	/*
	這裡之所以要wake_up_interruptible(&dev->inq)，是因為在user space reader可能呼叫poll system call一路call到driver poll func，
	在driver poll func會call poll_wait()，poll_wait()只會先讓reader加入wait queue，但是不會將reader enter block，reader是否enter 
	block取決於driver poll func的return mask，詳情可以參考miro ch6，我們畫的do_poll()裡有真正讓process enter block。 
	所以reader call driver poll func可能會enter block，所以需要一個東西去喚醒reader，在這裡就是用timer_fn(struct timer_list *t)
	這個timer callback func去喚醒 , 注意timer callback func是driver設定好，kernel定時執行的。
	*/
	// make readable per one timer interval
	atomic_set(&dev->can_rd, 1);
	wake_up_interruptible(&dev->inq);

	// make writable per two timer interval
	if (dev->timer_counter % 2) {
		atomic_set(&dev->can_wr, 1);
		wake_up_interruptible(&dev->outq);
	}

	/*
	每次call完，add_timer() timer都會從kernel移除，如果要週期性的call，就需要重新call add_timer()。
	*/
	dev->timer.expires = jiffies + TIMER_INTERVAL;
	add_timer(&dev->timer);
}

static int init_dev(struct poll_dev *dev)
{
    mutex_init(&dev->mutex);
	// timer_setup 是設定timer的callback func
    timer_setup(&dev->timer, timer_fn, 0);
    dev->timer.expires = jiffies + TIMER_INTERVAL;

    atomic_set(&dev->can_wr, 0);
	atomic_set(&dev->can_rd, 0);

    dev->timer_counter = 0;

    init_waitqueue_head(&dev->inq);
	init_waitqueue_head(&dev->outq);

    cdev_init(&dev->cdev, &fops);
	dev->cdev.owner = THIS_MODULE;

	dev->buf_len = ARRAY_SIZE(DFT_MSG);
	memcpy(dev->buff, DFT_MSG, dev->buf_len);

	return 0;
}

static int m_init(void)
{
    dev_t devno;
	int err = 0;

	printk(KERN_WARNING MODULE_NAME " is loaded\n");

	// 申請空間
    poll_dev = kmalloc(sizeof(struct poll_dev), GFP_KERNEL);
    if (!poll_dev) {
		pr_debug("Cannot alloc memory!\n");
		return -ENOMEM;
	}
	memset(poll_dev, 0, sizeof(struct poll_dev));

    // 申請設備號
    err = alloc_chrdev_region(&devno, poll_minor, POLL_DEV_NR, MODULE_NAME);
	if (err < 0) {
		pr_debug("Can't get major!\n");
		goto on_error;
	}
	poll_major = MAJOR(devno);

    init_dev(poll_dev);
    // 註冊device timer到system , 同時會開始計時，經過dev->timer.expires設定的時間後，執行callback func
    add_timer(&poll_dev->timer);

    // 註冊device cdev到system
    err = cdev_add(&poll_dev->cdev, devno, POLL_DEV_NR);
	if (err) {
		pr_debug("Error when adding ioctl dev");
		goto on_error;
	}

    return 0;

on_error:
	kfree(poll_dev);
	return err;
}

static void m_exit(void)
{
	dev_t devno;

	printk(KERN_WARNING "[KUO] module unloaded\n");

	cdev_del(&poll_dev->cdev);
	del_timer_sync(&poll_dev->timer);

	devno = MKDEV(poll_major, poll_minor);
	unregister_chrdev_region(devno, POLL_DEV_NR);

	kfree(poll_dev);
}

module_init(m_init);
module_exit(m_exit);