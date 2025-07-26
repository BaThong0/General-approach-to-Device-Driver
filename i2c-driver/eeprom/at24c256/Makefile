#GPIO Driver Legacy
BBB_KERNEL := /home/ton/Linux_Programming/BBB/kernelbuildscripts/KERNEL 
TOOLCHAIN := /home/ton/Linux_Programming/BBB/kernelbuildscripts/dl/gcc-8.5.0-nolibc/arm-linux-gnueabi/bin/arm-linux-gnueabi-

EXTRA_CFLAGS=-Wall
obj-m := at24c256.o

all: 
	make ARCH=arm CROSS_COMPILE=$(TOOLCHAIN) -C $(BBB_KERNEL) M=$(shell pwd) modules
clean:
	make -C $(BBB_KERNEL) M=$(shell pwd) clean