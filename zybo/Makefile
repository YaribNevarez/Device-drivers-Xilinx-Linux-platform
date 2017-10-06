obj-m=zybo.o

CPPFLAGS = -KERNEL_MODULE=\"Driver"

all:
	make -C ../../Linux-Digilent-Dev/ M=$(PWD) modules

clean:
	make -C ../../Linux-Digilent-Dev/ M=$(PWD) clean
