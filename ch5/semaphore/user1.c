#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

int main()
{
    int fd, result;

    printf("user1 test hello device_driver \n");
    /*open operation*/
    fd = open("/dev/hello", O_RDWR);
    
    if (fd < 0){
        perror("1. open failed \n");
    }else{
        printf("file opend\n");
    }

    /*write operation*/
    char w_b[12];
    /*strcpy(w_b,"1111111");
    result = write(fd, (void*) w_b, 12);
    if ( result != 0 ){
        printf("Oh dear, something went wrong with write()! %s, result = %d\n", strerror(errno), result);
    }
    else{
        printf("write operation executed succesfully\n");
    }*/

    /*read operation*/
    char a[1000];
    char b[1000];

    //reading a
    while(1){
        // result = write(fd, (void*) w_b, 12);
        if ( result != 0 ){
            printf("Oh dear, something went wrong with write()! %s, result = %d\n", strerror(errno), result);
        }
        //reading b
        result = read(fd, (void*)b, 7);
        if ( result != 0 ){
            printf("Oh dear, something went wrong with read()! %s, result = %d\n", strerror(errno), result);
        }
        else{
            printf("string read %s \n",b);
        }
        //sleep(5); 
    }

    /* Close operation */
    if (close(fd)){
		perror("1. close failed \n");
	}else{
		printf("file closed\n");
	}

	printf("-- TEST PASSED --\n");
    return 0;
}