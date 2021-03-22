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


struct myfile {
	struct file *f;
	mm_segment_t fs;
	loff_t pos;
};

struct myfile * open_file_for_read (char* filename) {
	struct myfile *fi = kmalloc (1024, GFP_DMA | GFP_KERNEL);

	fi->f = filp_open(filename, O_RDWR, 0);
	
	fi->fs = get_fs();
	set_fs(get_ds());
	set_fs(fi->fs);

	if (IS_ERR(fi->f)) {
		return NULL;
	}
	return fi;	
}

volatile int read_from_file_until(struct myfile * mf, char * buf, unsigned long vlen, char c){
	int cc;
	int i;
	int ret;
	char* tmpBuf;
	mm_segment_t fs = get_fs();

	cc = 0;
	i = 0;

	

	mf->fs=fs;
	set_fs(get_ds());

	tmpBuf = kmalloc(vlen, GFP_DMA | GFP_KERNEL);

	ret = vfs_read(mf->f, tmpBuf, 1, &mf->pos);

	
	while(tmpBuf[0] != c) {
			if(tmpBuf[0] == ' ' && cc < 2 && c=='('){
				memset(buf, 0, vlen);
				i = 0;
				cc++;
			}
			
			
				buf[i] = tmpBuf[0];
				i++;
				memset(tmpBuf, 0, vlen);
				ret = vfs_read(mf->f, tmpBuf, 1, &mf->pos);
				
				
	}

	

	set_fs(fs);
	return ret;
}

void close_file (struct myfile * mf) {
	filp_close(mf->f, NULL);
}

void remove(char* string) {
	    const char* data = string;
	        do {
		while (*data == ' ') {
			 ++data;
		}
		} while (*string++ = *data++);
}

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
	struct myfile *file;
	struct myfile *sysmap;

	int x;
	int y;
	int k;
	char* kernelVersion;
	char* sysmapfile;
	char* syscallAddress;
	char* entry;

	kernelVersion = kmalloc (1024, GFP_DMA | GFP_KERNEL);
	sysmapfile = kmalloc (1024, GFP_DMA | GFP_KERNEL);
	kernelVersion[strlen(kernelVersion)] = '\0';
	sysmapfile[strlen(sysmapfile)] = '\0';
	
	file = open_file_for_read("/proc/version");
	x = read_from_file_until(file, kernelVersion, 1024, '(');
	close_file(file);
	
	memset(sysmapfile, 0, 1024);
	strcpy(sysmapfile, "/boot/System.map-");
	remove(kernelVersion);
	strcat (sysmapfile, kernelVersion);
	
	sysmap = open_file_for_read(sysmapfile);
	syscallAddress = kmalloc (100, GFP_DMA | GFP_KERNEL);
	
	while(1) {
		entry = kmalloc (100, GFP_DMA | GFP_KERNEL);
		entry[strlen(entry)] = '\0';
		y = read_from_file_until(sysmap, entry, 100, '\n');
		if(strstr(entry, "sys_call_table") != NULL) {
			
			strncpy(syscallAddress, entry, 16);
			syscallAddress[strlen(syscallAddress)] = '\0';

			printk(KERN_INFO "sys_call address: %s\n", syscallAddress);
				unsigned long kPointer;
			
				sscanf(syscallAddress, "%lx", &kPointer);
				syscallTableAddress = (unsigned long *)kPointer;
				printk(KERN_INFO "sys_fork address: %lx\n", syscallTableAddress[__NR_clone]);
			break;
		}
	}
	close_file(sysmap);

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