#include <linux/module.h>
//#include <linux/init.h>
#include <linux/kernel.h>

static int __init morsecode_init(void) {
    printk(KERN_INFO "----> My test driver init()\n");
    return 0; 
}


static void __exit morsecode_exit(void)
{
      printk(KERN_INFO "<---- My test driver exit().\n");
}


// Link our init/exit functions into the kernel's code.
module_init(morsecode_init);
module_exit(morsecode_exit);

// Information about this module:
MODULE_AUTHOR("Cameron Savage");
MODULE_DESCRIPTION("A simple test driver"); 
MODULE_LICENSE("GPL"); // Important to leave as GPL.