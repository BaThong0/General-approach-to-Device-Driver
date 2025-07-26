#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

uint8_t writebuf[64];
uint8_t readbuf[64];

int main() {
    int fd;
    char option;
    fd = open("/dev/nokia5110", O_RDWR);
    if(fd < 0) {
        printf("Cannot open device file...\n");
        return 0;
    }
    while(1) {
        printf("Please enter the option\n");
        printf("1. Write\n");
        //printf("2. Read\n");
        printf("2.Exit\n");
        scanf("%c", &option);
        printf("Your option = %c\n", option);

        switch(option) {
            case '1': 
                printf("Enter the string to write into the driver: ");
                scanf(" %[^\t\n]s", writebuf);
                printf("Data Wrtiting ...");
                write(fd, writebuf, strlen(writebuf)+1);
                printf("Done!\n");
                break;
            //case '2':
                printf("Data Reading ...");
                read(fd, readbuf, 64);
                printf("Done!\n\n");
                printf("%s\n", readbuf);
                break;
            case '2':
                close(fd);
                exit(1);
                break;
            default:
                printf("Enter valid option = %c\n", option);
                break;
        }
    }
    close(fd);
}

