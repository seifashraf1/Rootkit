#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/gfp.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/unistd.h>
#include <linux/proc_fs.h>
#include <linux/kobject.h>
#include <linux/slab.h>


int fork_counter = 0;
long (*original_sys_fork) (struct pt_regs *regs);

asmlinkage long evil_fork (const struct pt_regs *regs) {
	fork_counter++;

	if (fork_counter%10 == 0)
	{
		printk(KERN_INFO "Fork Was Called: %d times\n", fork_counter);
	}

	original_sys_fork(regs);
}

unsigned long *syscallTableAddress;

int init_module (void) {

	syscallTableAddress = (void*) kallsyms_lookup_name("sys_call_table");

	write_cr0(read_cr0() & (~0x10000));

	original_sys_fork = syscallTableAddress[__NR_clone];

	syscallTableAddress[__NR_clone] = evil_fork;

	write_cr0(read_cr0() | 0x10000);

	return 0;
}

void cleanup_module(void) {
	
	write_cr0(read_cr0() & (~0x10000));
	
	syscallTableAddress[__NR_clone] = (void*) original_sys_fork;
	
	write_cr0(read_cr0() | 0x10000);

	printk(KERN_INFO "Module Removed\n");
}

MODULE_LICENSE("GPL");