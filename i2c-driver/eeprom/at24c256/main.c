#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "at24c256.h"

int main()
{
    int fd;
    int i = 0;
    char option, ch;
    fd = open("/dev/at24c256", O_RDWR);
    if (fd < 0)
    {
        printf("Cannot open device file...\n");
        return 0;
    }
    while (1)
    {
        uint8_t writebuf[PAGE_SIZE];
        uint8_t readbuf[PAGE_SIZE];
        //struct ioctl_image_data image_data;
        //struct ioctl_data data;
        uint8_t page_buffer[PAGE_SIZE];
        uint8_t read_buffer[PAGE_SIZE]; // Buffer to store read data
        uint8_t image_buffer[IMAGE_SIZE]; // Buffer to store read image data
        char write_data;
        uint8_t read_data;        // Buffer to store read data
        printf("\nPlease enter the option\n");
        printf("1. Write\n");
        printf("2. Read\n");
        printf("3. Byte write\n");
        printf("4. Page write\n");
        printf("5. Current read\n");
        printf("6. Sequential read\n");
        printf("7. Exit\n");
        option = getchar();
        while(option == '\n') {
            option = getchar();
        }
        printf("Your option = %c\n", option);
        fflush(stdin);
        switch (option)
        {
        case '1':
            printf("Enter the string to write into the driver: ");
            scanf("  %[^\t\n]s", writebuf);
            printf("Data Wrtiting ...");
            fflush(stdin);
            write(fd, writebuf, strlen(writebuf) + 1);
            printf("Done!\n");
            break;
        case '2':
            printf("Data Reading ...");
            read(fd, readbuf, 64);
            printf("Done!\n\n");
            printf("%s\n", readbuf);
            break;
        case '3': //byte write
            printf("Enter byte of data to write to the driver: ");
            write_data = getchar();
            while(write_data == '\n') {
                write_data = getchar();
            }
            printf("Data Wrtiting ...");
            fflush(stdin);
            if (ioctl(fd, BYTE_WRITE, &write_data) < 0)
            {
                perror("BYTE_WRITE failed");
                close(fd);
                return -1;
            }
            break;
        case '4': //page write
            printf("Enter 64 byte of data to the driver: ");
            scanf("  %[^\t\n]s", page_buffer);
            printf("Data Wrtiting ...");
            fflush(stdin);
            if (ioctl(fd, PAGE_WRITE, &page_buffer) < 0)
            {
                perror("PAGE_WRITE failed");
                close(fd);
                return -1;
            }
            break;
        case '5': //sequential read

            // Perform sequential read operation using IOCTL
            if (ioctl(fd, CURRENT_READ, &read_data) < 0)
            {
                perror("CURRENT_READ failed");
                close(fd);
                return -1;
            }

            printf("Current read successful. Data read:\n");
            printf("%02X ", read_data);
            printf("\n");
            break;
        case '6': //random read
            // Perform random read operation using IOCTL
            if (ioctl(fd, SEQUENTIAL_READ, &read_buffer) < 0)
            {
                perror("SEQUENTIAL_READ failed");
                close(fd);
                return -1;
            }

            printf("Random read successful. Data read:\n");
            for (int i = 0; i < PAGE_SIZE; ++i)
            {
                printf("%02X ", read_buffer[i]);
            }
            printf("\n");
            break;
            /*
        case '7': //image write
            for (int i = 0; i < IMAGE_SIZE; ++i)
            {
                image_buffer[i] = i; // Example: fill with incrementing values
            }
            // Perform random read operation using IOCTL
            if (ioctl(fd, WRITE_IMAGE, &image_buffer) < 0)
            {
                perror("IMAGE_WRITE failed");
                close(fd);
                return -1;
            }

            printf("Image write successfully:\n");
            break;
        case '8': //image read
            // Perform random read operation using IOCTL
            if (ioctl(fd, READ_IMAGE, &image_buffer) < 0)
            {
                perror("IMAGE_READ failed");
                close(fd);
                return -1;
            }

            printf("Image read successful. Image read:\n");
            for (int i = 0; i < IMAGE_SIZE; ++i)
            {
                printf("%02X ", image_buffer[i]);
            }
            printf("\n");
            break;
        */
        case '7':
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
