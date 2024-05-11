#include <linux/cdev.h>
#include <linux/semaphore.h>

#ifndef _COMMANDS_H_
#define _COMMANDS_H_

#define DEVICE_MAX_SIZE (20)
struct ioctl_01_dev {
	char *p_data_01;                 /* first memory buffer */
	char *p_data_02;                 /* second memory buffer */
	struct semaphore sem_ioctl_01;   /* semaphore for the struct hello */
	struct cdev cdev;	               /* Char device structure		*/
};

/*
 * Name of the buffers
 */
#define FIRST_BUFFER (1)
#define SECOND_BUFFER (2)

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

#endif