obj-m := flytouch.o
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
MODPATH := /lib/modules/$(shell uname -r)/kernel/drivers/input/touchscreen

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean

install:
	@echo "Installing FlyPOS Pro TouchScreen driver..."
	install -m 644 -v flytouch.ko $(MODPATH)

	@echo "Reloading module dependencies..."
	depmod -a

	@echo "Success!"

uninstall:
	@echo "Uninstalling FlyPOS Pro TouchScreen driver..."
	rm -f $(MODPATH)/flytouch.ko

	@echo "Reloading module dependencies..."
	depmod -a

	@if lsmod | grep -q flytouch; then \
		echo "Module is loaded, unloading..."; \
		rmmod flytouch; \
	fi
	@echo "Success."

.PHONY: all clean install uninstall