#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#include <linux/ioctl.h>

/*
 *   Debug Macros
 *
 */

#undef PDEBUG             /* undef it, just in case */
#ifdef LEO_DEBUG
#  ifdef __KERNEL__
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


/*
 * Ioctl definitions
 */

/* Use 'k' as magic number */
#define IOCTL_01_IOC_MAGIC  'T'
/* Please use a different 8-bit number in your code */

#define DEVICE_IOCRESET _IO(IOCTL_01_IOC_MAGIC, 0)
#define SET_FIRST_BUFFER _IO(IOCTL_01_IOC_MAGIC, 1)
#define SET_SECOND_BIFFER _IO(IOCTL_01_IOC_MAGIC, 2)
#define WHICH_BUFFER_USE _IOR(IOCTL_01_IOC_MAGIC, 3, int)
#define SET_WHICH_BUFFER_USE _IOW(IOCTL_01_IOC_MAGIC, 4, int)

#define IOCTL_01_IOC_MAXER (4)



#endif /* _COMMANDS_H_ */