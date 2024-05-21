#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#define BUFF_SIZE (1<<3)

int main(void)
{
    printf("read start\n");
    
    /*open*/
    int fd;
    fd = open("/dev/pipe_dev0", O_RDWR);
    if (fd < 0){
        printf("open failed \n");
    }else{
        printf("file opend\n");
    }

    /*read*/
    char buf[BUFF_SIZE];
    memset((void *)buf, 0, BUFF_SIZE);
    int result;
    result = read(fd, (void *)buf, 4);
    printf("string read %s \n", buf);

    printf("read over\n");
    return 0;
}