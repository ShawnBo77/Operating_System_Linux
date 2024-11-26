CONFIG_MODULE_SIG=n
obj-m += kfetch_mod_313551156.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

load:
	sudo insmod kfetch_mod_313551156.ko

unload:
	sudo rmmod kfetch_mod_313551156
