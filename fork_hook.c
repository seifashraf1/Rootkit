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

inline void mywrite_cr0(unsigned long cr0) {
  asm volatile("mov %0,%%cr0" : "+r"(cr0), "+m"(__force_order));
}

void enable_write_protection(void) {
  unsigned long cr0 = read_cr0();
  set_bit(16, &cr0);
  mywrite_cr0(cr0);
}

void disable_write_protection(void) {
  unsigned long cr0 = read_cr0();
  clear_bit(16, &cr0);
  mywrite_cr0(cr0);
}

int fork_counter = 0;
long (*original_sys_fork) (struct pt_regs *regs);

asmlinkage long evil_fork (const struct pt_regs *regs) {
	fork_counter++;
	/*printk(KERN_INFO "YOU HAVE BEEN HOOKED!\n");*/
	if (fork_counter%10 == 0)
	{
		printk(KERN_INFO "fork counter: %d \n", fork_counter);
	}

	original_sys_fork(regs);
}

unsigned long *syscall_table;

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

	/*kernelVersion = kmalloc (1024, GFP_DMA | GFP_KERNEL);
	sysmapfile = kmalloc (1024, GFP_DMA | GFP_KERNEL);

	kernelVersion[strlen(kernelVersion)] = '\0';
	sysmapfile[strlen(sysmapfile)] = '\0';
	
	sysmapfile = "/boot/System.map-4.19.0-13-amd64";

	file = open_file_for_read("/proc/version");
	x = read_from_file_until(file, kernelVersion, 1024, '(');
	close_file(file);
	
	memset(sysmapfile, 0, 1024);
	strcpy(sysmapfile, "/boot/System.map-");
	remove(kernelVersion);
	strcat (sysmapfile, kernelVersion);
	
	printk(KERN_INFO "System Map: %s\n", sysmapfile);


	sysmap = open_file_for_read(sysmapfile);

	syscallAddress = kmalloc (100, GFP_DMA | GFP_KERNEL);
	syscallAddress[strlen(syscallAddress)] = '\0';

	while(1) {
		entry = kmalloc (100, GFP_DMA | GFP_KERNEL);
		entry[strlen(entry)] = '\0';
		y = read_from_file_until(sysmap, entry, 100, '\n');

		if(strstr(entry, "sys_call_table") != NULL) {
			
			strncpy(syscallAddress, entry, 16);

			
			printk(KERN_INFO "sys_call address: %s\n", syscallAddress);

				unsigned long kPointer;
			
				sscanf(syscallAddress, "%lx", &kPointer);
				sys_call_ptr_t *sys_call_table;
				sys_call_table = (sys_call_ptr_t *)kPointer;
				printk(KERN_INFO "sys_fork address: %lx\n", sys_call_table[__NR_clone]);

			break;
		}
	}

	close_file(sysmap);*/

	

	syscall_table = (void*) kallsyms_lookup_name("sys_call_table");

	if (syscall_table == NULL) {
		printk (KERN_INFO "Could not find sys_call_table\n");
	} else {
		printk(KERN_INFO "sys_fork address: %lx\n", syscall_table[__NR_clone]);
	}

	/*disable_write_protection();*/

	write_cr0(read_cr0() & (~0x10000));

	original_sys_fork = syscall_table[__NR_clone];

	

	/*set_page_rw(syscall_table);*/

	syscall_table[__NR_clone] = evil_fork;

	/*original_sys_fork = (void *) xchg(&syscall_table[__NR_clone],evil_fork);*/

	write_cr0(read_cr0() | 0x10000);

	/*enable_write_protection();*/

	return 0;
}

void cleanup_module(void) {
	
	write_cr0(read_cr0() & (~0x10000));

	/*disable_write_protection();*/
	
	syscall_table[__NR_clone] = (void*) original_sys_fork;

	/*xchg(&syscall_table[__NR_clone],original_sys_fork);*/
	
	write_cr0(read_cr0() | 0x10000);

	/*enable_write_protection();*/

	printk(KERN_INFO "Module Removed\n");
}

MODULE_LICENSE("GPL");
