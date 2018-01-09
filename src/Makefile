obj-m+=tzm.o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
PWD := "$$(pwd)"

CFLAGS_$(obj-m) := -DDEBUG -g3

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean

insmod:
	insmod tzm.ko
rmmod:
	rmmod tzm

