# Author: Yohan Hmaiti, Anna Zheng, Alyssa Yeekee

obj-m += pa2OS_out.o

obj-m += pa2OS_in.o



# when we run make only 

all:

	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules



# to clean 

clean:

	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean