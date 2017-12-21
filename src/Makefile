obj-m+=tzm.o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
PWD := "$$(pwd)"

CFLAGS_$(obj-m) := -DDEBUG -g3

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

insmod:
	insmod tzm.ko
rmmod:
	rmmod tzm

