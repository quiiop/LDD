#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "seq_file.h"



MODULE_AUTHOR("Leonardo Suriano <leonardo.suriano@live.it>");
MODULE_LICENSE("GPL");

const static char *PROC_NAME;
static struct proc_dir_entry *entry;

/*
/proc/序列文件, 在linux由struct seq_file表示 , struct seq_file是一個跌代器

struct seq_file {
    char *buf;  //序列文件对应的数据缓冲区，要导出的数据是首先打印到这个缓冲区，然后才被拷贝到指定的用户缓冲区。
    size_t size;  //缓冲区大小，默认为1个页面大小，随着需求会动态以2的级数倍扩张，4k,8k,16k...
    size_t from;  //没有拷贝到用户空间的数据在buf中的起始偏移量
    size_t count; //buf中没有拷贝到用户空间的数据的字节数，调用seq_printf()等函数向buf写数据的同时相应增加m->count
    size_t pad_until; 
    loff_t index;  //正在或即将读取的数据项索引，和seq_operations中的start、next操作中的pos项一致，一条记录为一个索引
    loff_t read_pos;  //当前读取数据（file）的偏移量，字节为单位
    u64 version;  //文件的版本
    struct mutex lock;  //序列化对这个文件的并行操作
    const struct seq_operations *op;  //指向seq_operations
    int poll_event; 
    const struct file *file; // seq_file相关的proc或其他文件
    void *private;  //指向文件的私有数据
};
*/

/*
流程
1. 創建序列文件使用proc_create( )
2. 定義/proc/序列文件的struct file_operations
3. 定義struct seq_file的struct seq_operations
4. 定義移除module function , 需要移除/proc/序列文件
*/

/*seq_file的目的是查看array or linklist , 所以力片的中止條件自己寫*/

/*
    loff_t   = long long
    uint32_t = unsigned int
*/ 

#define SIZE 10
static int *Array;

/*.start()的目的是返回iter, 也就是seq_file開始reading的position*/
static void *my_start(struct seq_file *m, loff_t *pos) // pos是現在的位置
{
    unsigned long *array_index;
    if (*pos >= SIZE){
        PDEBUG("[my_start]: pos >= SIZE\n");
        return NULL;
    }
    
    array_index = kmalloc(sizeof(unsigned long), GFP_KERNEL);
    if (!array_index){
        return NULL;
    }

    *array_index = *pos +1;

	return array_index;
}

/* *v為現在read buffer的位置 , pos為 */
static void *my_next(struct seq_file *m, void *v, loff_t *pos)
{
    unsigned long *array_index = (unsigned long *)v;
    
    if (++(*array_index) >= SIZE){
        PDEBUG("[my_start]: array_index >= SIZE\n");
    }
    return NULL;
}

static void my_stop(struct seq_file *m, void *v)
{
    for (int i=0; i<SIZE; i++){
        Array[i] = 0;
    }
    kfree(v);
    PDEBUG("[my_stop]: free sequence file\n");
}

static int my_show(struct seq_file *m, void *v) // v是現在的跌代器
{
    unsigned long *array_index = (unsigned long *)v;
    /*void seq_printf(struct seq_file *m, const char *fmt, ...);*/
    seq_printf(m, "Array[%ld] = %d\n", *array_index, Array[(*array_index)]);
    PDEBUG("[my_show]: show data\n");
    return 0;
}

/*
struct seq_operations {
	void * (*start) (struct seq_file *m, loff_t *pos);
	void (*stop) (struct seq_file *m, void *v);
	void * (*next) (struct seq_file *m, void *v, loff_t *pos);
	int (*show) (struct seq_file *m, void *v);
};
*/
static struct seq_operations my_seq_ops = {
    .start = my_start,
    .stop = my_stop,
    .next = my_next,
    .show = my_show,
};

static int my_open(struct inode *inode, struct file *file)
{
    PDEBUG("opening proc/ file \n");
    /*
    從int seq_open(struct file *file, const struct seq_operations *op)的定義
    可以看出struct seq_file是系統動態創造的結構 , struct seq_file被創建後會被struct file->private_date管理
    這樣系統就可以找到struct seq_file
    */
    return seq_open(file, &my_seq_ops);
}

/*
seq_read, seq_lseek, seq_release 都是#include <linux/seq_file.h>用以操作struct seq_file
linux kernel 6.5.0的/proc/序列文件的file operation改用struct proc_ops表示
*/
static struct proc_ops my_file_ops = {
    .proc_open = my_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = seq_release 
};

static void KUO_cleanup_module(void)
{
    remove_proc_entry(PROC_NAME, entry);
}

/*
struct proc_dir_entry *proc_create(const char *name, umode_t mode,
				                    struct proc_dir_entry *parent,
				                    const struct proc_ops *proc_ops)
proc_create( )會負責建立/proc/序列文件
*/
static int __init KUO_init_module(void)
{
    /*/proc/序列文件的文件名稱*/
    PROC_NAME = "KUO_seq_file";
    
    PDEBUG("[KUO init_module] int module function \n");
    entry = proc_create(PROC_NAME,0,
                        NULL,
                        &my_file_ops);
    if (!entry){
        printk(KERN_INFO "[KUO] Proc create failed\n");
        return -ENOMEM;
    }

    Array = kmalloc(SIZE*sizeof(int), GFP_KERNEL);
    for (int i=0; i<SIZE; i++){
        Array[i] = i;
    }
    return 0;
}

static void __exit KUO_exit_module(void)
{
    printk(KERN_INFO "[KUO] --calling cleanup function--\n");
	KUO_cleanup_module();
}

module_init(KUO_init_module);
module_exit(KUO_exit_module);