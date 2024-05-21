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

int pipe_open(struct inode *inode, struct file *filp)
{
    struct pipe_dev *my_dev;
    my_dev = container_of(inode->i_cdev, struct pipe_dev, cdev);
    if (!my_dev){
        printk(KERN_INFO "[KUO] open failed\n");
        return -1;
    }
    
    filp->private_data = my_dev;
    printk(KERN_INFO "[KUO] open successful , file major %u, minor %u\n", MAJOR(my_dev->cdev.dev), MINOR(my_dev->cdev.dev));
    return 0;
}

/*pipe_read 當buffer有資料時，不會block，但是如果buffer是空的，就enter block*/
/*read ops : f_pos表示從哪裡開始讀資料*/
ssize_t pipe_read(struct file *filp, char __user * buff, size_t count, loff_t * f_pos)
{
    printk(KERN_INFO "[KUO] perform read ops\n");

    ssize_t retval;
    struct pipe_dev *my_dev = filp->private_data;
    if (!my_dev){
        printk(KERN_INFO "[KUO] perform read failed\n");
        return -1;
    }

    /*wait_event_interruptible、wait_event_interruptible 如果回傳非0，表示被interrupt打斷，像是ctl+c*/
    /*1. 因為buffer是shared data，所以先取得mutex*/
    if (mutex_lock_interruptible(&my_dev->mutex)){
        return -ERESTARTSYS;
    }
    
    /*2. 如果buffer是空的enter while，並且進行wait buffer有資料的event，
        記得在任何可能會導致process enter block的操作前，都應該要release mutex or semaphore*/
    while(!my_dev->buff_len){
        mutex_unlock(&my_dev->mutex);
        
        if (filp->f_flags & O_NONBLOCK){
            return -EAGAIN;
        }

        printk(KERN_INFO "read : process %d(%s) is going to sleep\n", current->pid, current->comm);

        /*if condition is not true , process will enter block*/
        if (wait_event_interruptible(my_dev->rd_queue, (my_dev->buff_len > 0))){
            return -ERESTARTSYS;
        }

        /*如果process從wr_queue wake up，需要重新獲得mutex lock，並且從wr_queue wake up時，表示buffer_len > 0(有資料)*/
        if (mutex_lock_interruptible(&my_dev->mutex)){
            return -ERESTARTSYS;
        }
    }

    /*正常*/
    if (count > (my_dev->buff_len - *f_pos)){
        count = my_dev->buff_len - *f_pos;
    }

    /*my_dev->buff[*f_pos] 等於 my_dev->buff+*f_pos*/
    printk(KERN_INFO "[KUO] read ops : current f_pos = %lld\n", *f_pos);
    if (copy_to_user(buff, my_dev->buff+*f_pos, count)){
        printk(KERN_INFO "copy to user err\n");
        retval = -EFAULT;
        goto copy_error;
    }

    /*如果能走到這，表示copy_to_user動作完成，要不然失敗會goto copy_error*/
    printk(KERN_INFO "[KUO] read ops : f_pos= %lld, count= %lu, buff_len= %d\n", *f_pos, count, my_dev->buff_len);

    /*更新目前讀到的位置*/
    *f_pos += count;

    /*如果判斷式為true，表示資料已經讀完了*/
    if (*f_pos > my_dev->buff_len){
        my_dev->buff_len = 0;
        *f_pos = 0;
        printk(KERN_INFO "read: process %d awakening the writers...\n", current->pid);
        
        /*資料讀完，所以要wake up wr_queue的process*/
        wake_up_interruptible(&my_dev->wr_queue);
    }

    retval = count;
    mutex_unlock(&my_dev->mutex);
    return retval;

copy_error:
    mutex_unlock(&my_dev->mutex);
    return retval;
}

/*
write的寫入流程
1. 嘗試mutex lock。
2. 成功lock mutex，確認buffer是否為空。
3. buffer不為空，write process enter block，反之buffer為空，write process start execute。
4. 因為buffer是shared data，所以操作buffer需要上鎖，acquire mutex。
5. write data in buffer
6. leave buffer，release mutex lock and wake up sleep process in read queue。
*/
/*write ops : f_pos表示從哪裡開始寫資料*/
ssize_t pipe_write(struct file *filp, const char __user * buff, size_t count, loff_t * f_pos)
{
    printk(KERN_INFO "[KUO] perform write ops\n");
    
    int retval = count;
    struct pipe_dev *my_dev = filp->private_data;

    /*1. 嘗試mutex lock。*/
    if (mutex_lock_interruptible(&my_dev->mutex)){
        return -ERESTARTSYS;
    }

    /*2. 成功lock mutex，確認buffer是否為空。*/
    /*3. buffer不為空，write process enter block，反之buffer為空，write process start execute。*/
    while(my_dev->buff_len){
        mutex_unlock(&my_dev->mutex);

        /*如果struct file被設定成non-block，那自然不能讓process enter sleep*/
        if (filp->f_flags & O_NONBLOCK){
            return -EAGAIN;
        }

        printk(KERN_INFO "write : process %d is going to sleep\n", current->pid);

        if (wait_event_interruptible(my_dev->wr_queue, my_dev->buff_len==0)){
            return -ERESTARTSYS;
        }

        /*4. 因為buffer是shared data，所以操作buffer需要上鎖，acquire mutex。*/
        if (mutex_lock_interruptible(&my_dev->mutex)){
            return -ERESTARTSYS;
        }
    }

    /*5. write data in buffer*/
    /*(BUFFER_SIZE - *f_pos)表示剩餘空間*/
    if (count > BUFF_SIZE - *f_pos){
        count  = BUFF_SIZE - *f_pos;
    }

    printk(KERN_INFO "[KUO] write ops : current f_pos = %lld\n", *f_pos);
    if (copy_from_user(my_dev->buff+*f_pos, buff, count)){
        printk(KERN_INFO "[KUO] copy to user err\n");
        retval = -EFAULT;
        goto copy_error;
    }

    printk(KERN_INFO "[KUO] write ops : f_pos= %lld, count= %lu, buff_len= %d\n", *f_pos, count, my_dev->buff_len);

    /*count > 0表示真的有寫入資料到buffer，而且沒有goto copy_error表示有成功寫入資料到buffer*/
    if (count > 0){
        my_dev->buff_len = count;
        *f_pos = 0;
        printk(KERN_INFO "[KUO] write ops : process (%d) awakeing the readers ...\n", current->pid);

        /*因為有成功寫入資料，才wake up sleeping process in read queue*/
        wake_up_interruptible(&my_dev->rd_queue);
    }

    retval = count;
    mutex_unlock(&my_dev->mutex);
    return retval;

/*即便失敗，也要release mutex*/
copy_error:
    mutex_unlock(&my_dev->mutex);
	return retval;
}