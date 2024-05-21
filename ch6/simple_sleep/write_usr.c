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
        printf("1. open failed \n");
    }else{
        printf("file opend\n");
    }

    /*write*/
    char buf[BUFF_SIZE];
    memset((void *)buf, 0, BUFF_SIZE);
    for (int i=0; i<BUFF_SIZE; i++){
        char tmp = 'A'+i;
        buf[i] = tmp;
    }

    int result;
    result = write(fd, (void *)buf, BUFF_SIZE);
    printf("string write %s \n", buf);

    printf("write over\n");
    return 0;
}