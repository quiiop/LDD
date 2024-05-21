#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>

#include "main.h"
#include "fops.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("d0u9");
MODULE_DESCRIPTION("A pipe like device to illustrate the skill of how to put"
		   "the read/write process into sleep");

static int pipe_major = 0, pipe_minor = 0;
static struct pipe_dev *pipe_dev[PIPE_DEV_NR]; //共4個設備

static struct file_operations fops={
    .open = pipe_open,
    .read = pipe_read,
    .write = pipe_write,
};

/*
struct pipe_dev{
    struct cdev cdev;
    struct mutex mutex;
    wait_queue_head_t rd_queue;
    wait_queue_head_t wr_queue;
    int buff_len; //buff_len 表示目前buffer擁有的資料量
    char buff[BUFF_SIZE];
};
*/

static void init_pipe_dev(struct pipe_dev *my_dev, dev_t tmp_devno)
{   
    memset(my_dev, 0, sizeof(struct pipe_dev));
    /*mutex init*/    
    mutex_init(&my_dev->mutex);
    /*cdev init*/
    cdev_init(&my_dev->cdev, &fops);
    cdev_add(&my_dev->cdev, tmp_devno, 1);
    /*init wait queue*/
    init_waitqueue_head(&my_dev->rd_queue);
    init_waitqueue_head(&my_dev->wr_queue);
}

static int m_init(void)
{
    dev_t devno;
    int err = 0;

    /*申請設備號*/
    printk(KERN_INFO "module init\n");
    err = alloc_chrdev_region(&devno, pipe_minor, PIPE_DEV_NR, "pipe_dev"); // "pipe_dev = device name"
    if (err<0){
        printk(KERN_INFO "module get failed\n");
        return err;
    }
    pipe_major = MAJOR(devno);
    printk(KERN_INFO "module get successful, major number %d\n", pipe_major);
    
    /*別忘了，這裡我們一口氣申請多個設備*/
    for (int i=0; i<PIPE_DEV_NR; i++){
        pipe_dev[i] = kmalloc(sizeof(struct pipe_dev), GFP_KERNEL);
        if (!pipe_dev[i]){
            printk(KERN_INFO "can't get pipe_dev[i] space\n");
        }

        dev_t tmp_devno = MKDEV(pipe_major, pipe_minor+i); 
        init_pipe_dev(pipe_dev[i], tmp_devno);
    }

    return 0;
}

static void m_exit(void)
{
    dev_t devno;
    printk(KERN_INFO "module exit\n");

    for (int i=0; i<PIPE_DEV_NR; i++){
        cdev_del(&pipe_dev[i]->cdev);
        kfree(pipe_dev[i]);
    }

    devno = MKDEV(pipe_major, pipe_minor);
    unregister_chrdev_region(devno, PIPE_DEV_NR);
}

module_init(m_init);
module_exit(m_exit);



