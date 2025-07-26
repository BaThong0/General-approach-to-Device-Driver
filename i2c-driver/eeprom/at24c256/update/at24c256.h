#ifndef __AT24C256_H__
#define __AT24C256_H__

#define PAGE_SIZE 64
#define IMAGE_SIZE 504
struct data {
    int address;
    uint8_t chunk[IMAGE_SIZE];
};

#define BYTE_WRITE _IOW('g', 1, uint8_t*)
#define PAGE_WRITE _IOW('g', 2, uint8_t*)
#define CURRENT_READ _IOR('g', 3, uint8_t *)
#define SEQUENTIAL_READ _IOR('g', 4, uint8_t*)
#define RANDOM_WRITE _IOW('g', 5, struct data*)
#define RANDOM_READ _IOWR('g', 6, struct data*)
#define RANDOM_WRITE_BYTE _IOW('g', 7, struct data*)
#define RANDOM_READ_BYTE _IOWR('g', 8, struct data*)

#endif // __TESTNOKIA5110_H__