HOW TO COMPILE THIS KERNEL MODULE?

1- Make
2- insmod fork_hook.ko
	-you will find in "dmesg | tail" the syscall and fork address from sysmap and the fork counter each 10 times
3- rmmod fork_hook.ko
	-to remove the module 


NOTE: KERNEL may crashes if you reinserted the module multiple times.


SEIF ASHRAF