#include <linux/cdev.h>

#ifndef __HELLO_H
#define __HELLO_H

#define DEVICE_MAX_SIZE (20)

struct hello_dev {
    char *p_data;
    struct cdev cdev; // 這裡不能寫char cdev *cdev, 不然container_of會抓不到hello_dev
}; 

#endif