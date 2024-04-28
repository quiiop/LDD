#ifndef _SEQ_FILE_H_
#define _SEQ_FILE_H_

#undef PDEBUG             /* undef it, just in case */
#ifdef LEO_DEBUG // 先設定是否允許DEBUG訊息
#  ifdef __KERNEL__ // 設定DEBUG訊息輸出在kernel space or user space
     /* This one if debugging is on, and kernel space */
#    define PDEBUG(fmt, args...) printk( KERN_DEBUG "[LEO] " fmt, ## args)
#  else
     /* This one for user space */
#    define PDEBUG(fmt, args...) fprintf(stderr, fmt, ## args)
#  endif /* __KERNEL__ */
#else
#  define PDEBUG(fmt, args...) /* not debugging: nothing */
#endif /* LEO_DEBUG */

#undef PDEBUGG
#define PDEBUGG(fmt, args...) /* nothing: it's a placeholder */

#define MAX_LINE_PRINTED (5) // struct seq_file->buffer最大可以紀錄的數量


#endif /* _SEQ_FILE_H_ */