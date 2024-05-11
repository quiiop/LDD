#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "test_ioctl_01.h"

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

int main(void)
{
    int fd, result;
    int buffer_in_use;
    printf("TEST ioctl_01 device_driver--\n");
    printf("DEVICE_IOCRESET %u\n", DEVICE_IOCRESET);
    printf("WHICH_BUFFER %lu\n", WHICH_BUFFER_USE);
    printf("SET_WHICH_BUFFER_USE %lu\n", SET_WHICH_BUFFER_USE);

    /*open device*/
    fd = open("/dev/ioctl_01", O_RDWR);
    if (fd < 0){
        perror("1. open failed \n");
        goto fail;
    }else{
        printf("file opend\n");
    }

    /*1. DEVICE_IOCRESET*/
    /*DEVICE_IOCRESET buffer_in_use=0*/
    int value_ioctl = ioctl(fd,SET_FIRST_BUFFER);
    if (value_ioctl < 0){
        printf("value_ioctl : %d\n", value_ioctl);
        printf(" %s\n", strerror(errno));
    }

    /*2. WHICH_BUFFER_USE 查看現在用哪個buffer*/
    ioctl(fd, SET_WHICH_BUFFER_USE, &buffer_in_use);
    printf("now is user biffer_%d\n", buffer_in_use);
    printf("buffer addr %p\n", &buffer_in_use);

    /*3. SET_WHICH_BUFFER_USE 設定你要用的buffer*/
    buffer_in_use = 2;
    ioctl(fd, SET_WHICH_BUFFER_USE, &buffer_in_use);

    /*4. write data in setting buffer*/
    char w_b[12];
    strcpy(w_b,"HI BUf2");
    result = write(fd, (void*)w_b, 12);
    if ( result != 0 ){
        printf("Oh dear, something went wrong with write()! %s\n", strerror(errno));
    }
    else{
        printf("write operation executed succesfully\n");
    }
    
    /*5. WHICH_BUFFER_USE 查看現在用哪個buffer*/
    ioctl(fd, SET_WHICH_BUFFER_USE, &buffer_in_use);
    printf("now is user biffer_%d\n", buffer_in_use);

    return 0;
    
    fail:
        return -1;
}



