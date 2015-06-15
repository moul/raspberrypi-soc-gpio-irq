KERNEL_HEADERS	= /lib/modules/$(shell uname -r)/build
FIRMWARE_COMMIT := $(shell zgrep "* firmware as of" /usr/share/doc/raspberrypi-bootloader/changelog.Debian.gz | head -1 | awk '{ print $5 }')

obj-m := raspberry-gpio-irq.o

all: /usr/src/linux-$(shell uname -r)
	@$(MAKE) -C $(KERNEL_HEADERS) M=$(PWD) modules

clean:      
	@$(MAKE) -C $(KERNEL_HEADERS) M=$(PWD) clean

/usr/src/linux-$(shell uname -r):
	cd /usr/src; sudo wget https://github.com/raspberrypi/linux/archive/$(FIRMWARE_COMMIT).tar.gz
	cd /usr/src; sudo tar -xzf $(FIRMWARE_COMMIT).tar.gz
	sudo mv /usr/src/linux-$(FIRMWARE_COMMIT) $@
	sudo ln -s $@ /usr/src/linux
	sudo ln -s $@ /lib/modules/$(shell uname -r)/build