#include <linux/module.h>       
#include <linux/miscdevice.h>   //
#include <linux/kernel.h>       //
#include <linux/fs.h>           // 
#include <linux/leds.h>
#include <linux/uaccess.h>      // copy_to_user
#include <linux/slab.h>         // kmalloc

// Solved VS Code IntelliSense issues with https://github.com/microsoft/vscode-cpptools/issues/5588

#define MY_DEVICE_FILE "morse-code"
#define DOT_TIME_NS 200000000
DEFINE_LED_TRIGGER(my_trigger);

static int my_open(struct inode *inode, struct file *file);
static int my_close(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char *buff, size_t count, loff_t *ppos);
static ssize_t my_write(struct file *file, const char *buff, size_t count, loff_t *ppos);

static int __init morsecode_init(void);
static void __exit morsecode_exit(void);


static struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close, 
    .read = my_read,
    .write = my_write
};

static struct miscdevice my_miscdevice = { 
    .minor = MISC_DYNAMIC_MINOR,
    .name = MY_DEVICE_FILE,
    .fops = &my_fops,
    .mode = 0666
};


static int my_open(struct inode *inode, struct file *file) {
    return 0;
}

static int my_close(struct inode *inode, struct file *file) {
    return 0;
}

static ssize_t my_read(struct file *file, char *buff, size_t count, loff_t *ppos) {
    return 0;
}

static ssize_t my_write(struct file *file, const char *buff, size_t count, loff_t *ppos) {
    // TODO Section 1.2
   
    // copy_from_user() into local kernel buffer, return w/ error if needed
    char *localBuff = kmalloc(count * sizeof(const char), GFP_KERNEL);

    if (localBuff == NULL) {
        printk(KERN_INFO "ERROR: failed to allocate kernel buffer\n");
    }

    if (copy_from_user(localBuff, buff, count)) {
        // CASE: could not copy all data from buff -> localBuff
        
    }


    // loop: iterate through buffer
    //   copy character from buffer to local char variable
    //   if invald character (not a-z, A-Z, " "), skip
    //   //if character is a space, wait for "seven dot-times"

    //   flash pattern for character
    //      - loop for # of bits for character:
    //          - read leftmost bit
    //          - if 1, LED on, if 0, LED off
    //          - if not last bit, wait for DOT_TIME_NS 

    //   wait 3 * DOT_TIME_NS before going to next character
    //      - reduce space waiting time to 7-3 = 4 DT? 3 is waited anyway for each char

    // iterate ppos by count (or however many characters read)
    // return count (or however many characters read)
    
    return 0;
}


static int __init morsecode_init(void) {
    int ret;
    ret = misc_register(&my_miscdevice);

    led_trigger_register_simple("morse-code", &my_trigger);
    printk(KERN_INFO "----> init LED trigger morse-code\n");

    return ret; 
}

static void __exit morsecode_exit(void) {
    misc_deregister(&my_miscdevice);

    led_trigger_unregister_simple(my_trigger);
    printk(KERN_INFO "<---- exit LED trigger morse-code.\n");
}


// Link our init/exit functions into the kernel's code.
module_init(morsecode_init);
module_exit(morsecode_exit);

// Information about this module:
MODULE_AUTHOR("Cameron Savage");
MODULE_DESCRIPTION("Trigger for onboard LEDs that flashes in morse code"); 
MODULE_LICENSE("GPL"); // Important to leave as GPL.