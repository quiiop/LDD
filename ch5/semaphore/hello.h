#include <linux/semaphore.h>
#include <linux/cdev.h>

#ifndef _HELLO_H_
#define _HELLO_H_

#define DEVICE_MAX_SIZE (20)

struct hello_dev {
    char *p_data;
    struct semaphore sem_hello;
    struct cdev cdev;
};

#endif