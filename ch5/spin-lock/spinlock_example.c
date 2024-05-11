#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h> // struct task_struct
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include <linux/spinlock_types.h>

#define LOOPS 100000

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Miroslav Tisma <tisma@linux.com>");
MODULE_DESCRIPTION("Simple kernel module which demonstrates using of spinlock");

static int list[LOOPS];
/*
idx是總執行次數
cs1是thread1執行的次數
cs2是thread2執行的次數
*/
static int idx = 0, cs1 = 0, cs2 = 0;
static struct task_struct *t1, *t2;
static spinlock_t spinlock;

static int consumer(void* thread_id)
{
    unsigned long flags = 0;
    printk(KERN_INFO "[KUO] Consumer TID %d\n", (int)thread_id);

    while(1)
    {
        spin_lock_irqsave(&spinlock, flags);
        if (idx >= LOOPS){
            spin_unlock_irqrestore(&spinlock, flags);
            return 0;
        }
        /*
        分解動作
        list[idx] = list[idx] +1
        idx++
        */
        list[idx++] += 1;
        /*
        讓另一個thread有機會搶到spin lock
        */
        spin_unlock_irqrestore(&spinlock, flags);
        if ((int)thread_id == 1){
            cs1++;
        }else{
            cs2++;
        }
    }

    printk(KERN_INFO "[KUO] Consumer %d done\n", (int)thread_id);
    return 0;
}

static int spinlock_init(void)
{
    int i;
    int thread_1_id = 1;
    int thread_2_id = 2;
    int lo_cnt = 0;
    int hi_cnt = 0;

    for (i=0; i<LOOPS; i++){
        list[i] = 0;
    }

    spin_init_lock(&spinlock);

    t1 = kthread_create(consumer, (void *)thread_1_id, "thread 1");
    t2 = kthread_create(consumer, (void *)thread_2_id, "thread 2");
}

void spinlock_cleanup(void)
{
    printk(KERN_INFO "[KUO] Inside cleanup_module\n");
}

module_init(spinlock_init);
module_exit(spinlock_cleanup);