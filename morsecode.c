#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/leds.h>

#define MY_DEVICE_FILE "morse-code"
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
    return 0;
}


static int __init morsecode_init(void) {
    led_trigger_register_simple("morse-code", &my_trigger);
    printk(KERN_INFO "----> init LED trigger morse-code\n");
    return 0; 
}

static void __exit morsecode_exit(void) {
    led_trigger_unregister_simple(my_trigger);
    printk(KERN_INFO "<---- exit LED trigger morse-code.\n");
}


// Link our init/exit functions into the kernel's code.
module_init(morsecode_init);
module_exit(morsecode_exit);

// Information about this module:
MODULE_AUTHOR("Cameron Savage");
MODULE_DESCRIPTION("A simple test driver"); 
MODULE_LICENSE("GPL"); // Important to leave as GPL.