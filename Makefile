
KERNEL_SRC = rs_driver.c

APP_SRC = rs_app.c

obj-m += $(KERNEL_SRC:.c=.o)

KDIR = /lib/modules/$(shell uname -r)/build

all: rs_driver rs_app

rs_driver:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

rs_app: $(APP_SRC) rs_driver_ioctl.h
	gcc -o rs_app $(APP_SRC)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f rs_app