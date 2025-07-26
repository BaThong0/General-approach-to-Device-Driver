obj-m += fpc1020a_driver.o

KDIR = /lib/modules/$(shell uname -r)/build

all: module dt
	echo Built Device Tree Overlay and Kernel Module
	
module:
	make -C $(KDIR) M=$(shell pwd) modules

dt: fpc1020a_overlay.dts
	dtc -@ -I dts -O dtb -o fpc1020a_overlay.dtbo fpc1020a_overlay.dts

clean:
	make -C $(KDIR) M=$(shell pwd) clean
	rm -rf fpc1020a_overlay.dtbo
