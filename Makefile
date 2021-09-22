abc-objs := tcp_abc.o
abccubic-objs := tcp_abccubic.o
obj-m := abc.o abccubic.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
