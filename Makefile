KERNEL_HEADERS	=	/lib/modules/$(shell uname -r)/build
FIRMWARE_COMMIT :=	$(shell zgrep "* firmware as of" /usr/share/doc/raspberrypi-bootloader/changelog.Debian.gz | head -1 | awk '{ print $$5 }')
LINUX_COMMIT :=		$(shell curl -s https://raw.githubusercontent.com/raspberrypi/firmware/$(FIRMWARE_COMMIT)/extra/git_hash)

obj-m := raspberry-gpio-irq.o


all: /usr/src/linux-$(shell uname -r)
	@$(MAKE) -C $(KERNEL_HEADERS) M=$(PWD) modules


clean:      
	@$(MAKE) -C $(KERNEL_HEADERS) M=$(PWD) clean


/usr/src/linux-$(shell uname -r):
	@echo "Download sources"
	cd /usr/src; sudo wget https://github.com/raspberrypi/linux/archive/$(LINUX_COMMIT).tar.gz
	cd /usr/src; sudo tar -xzf $(LINUX_COMMIT).tar.gz
	sudo mv /usr/src/linux-$(LINUX_COMMIT) $@
	sudo ln -s $@ /usr/src/linux
	sudo ln -s $@ $(KERNEL_HEADERS)

	@echo "Prepare kernel for module building"
	sudo make -C $(KERNEL_HEADERS) mrproper
	sudo sh -c 'zcat /proc/config.gz  > $(KERNEL_HEADERS)/.config'
	cd $(KERNEL_HEADERS); sudo wget https://github.com/raspberrypi/firmware/raw//extra/Module.symvers
	sudo make -C $(KERNEL_HEADERS) modules_prepare
