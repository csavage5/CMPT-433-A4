/*
 * demo_miscdrv.c
 * - Demonstrate how to use a misc driver to easily create a file system link
 *      Author: Brian Fraser
 */
#include <linux/module.h>
#include <linux/miscdevice.h>		// for misc-driver calls.
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

//#error Are we building this?

#define MY_DEVICE_FILE  "my_demo_misc"

// Define the toy data:
#define START_CHAR 'A'
#define END_CHAR   'z'
#define DATA_SIZE  (END_CHAR - START_CHAR + 1)
static char data[DATA_SIZE];


/******************************************************
 * Initialization of Data Function
 ******************************************************/
static void initialize_data(void)
{
	int i = 0;
	for (i = 0; i < DATA_SIZE; i++) {
		data[i] = i + START_CHAR;
	}
}

/******************************************************
 * Callbacks
 ******************************************************/
static ssize_t my_read(struct file *file,
		char *buff, size_t count, loff_t *ppos)
{
	int buffer_idx = 0;
	int data_idx = (int) *ppos;
	int bytes_read = 0;

	printk(KERN_INFO "demo_miscdrv::my_read(), buff size %d, f_pos %d\n",
			(int) count, (int) *ppos);

	// Fill buffer 1 byte at a time
	for (buffer_idx = 0; buffer_idx < count; buffer_idx++) {

		if (data_idx >= DATA_SIZE) {
			break;
		}

		// Copy next byte of data into user's buffer.
		// copy_to_user returns 0 for success (# of bytes not copied)
		if (copy_to_user(&buff[buffer_idx], &data[data_idx], 1)) {
			printk(KERN_ERR "Unable to write to buffer.");
			return -EFAULT;
		}


		data_idx++;
		bytes_read++;
	}

	// Write to in/out parameters and return:
	*ppos = data_idx;
	return bytes_read;  // # bytes actually read.
}

static ssize_t my_write(struct file *file,
		const char *buff, size_t count, loff_t *ppos)
{
	int buff_idx = 0;
	char min_char = 0;

	printk(KERN_INFO "demo_miscdrv: In my_write()\n");

	// Find min character
	for (buff_idx = 0; buff_idx < count; buff_idx++) {
		char ch;
		// Get the character
		if (copy_from_user(&ch, &buff[buff_idx], sizeof(ch))) {
			return -EFAULT;
		}

		// Skip special characters:
		if (ch <= ' ') {
			continue;
		}

		// Process the character
		if (buff_idx == 0 || ch < min_char) {
			min_char = ch;
		}		
	}

	// Print out the message
	if (count >= 0) {
		printk(KERN_INFO "    Min character is %c (#%d).\n", min_char, min_char);
	} else {
		printk(KERN_INFO "    No characters in buffer to analyze.\n");
	}

	// Return # bytes actually written.
	*ppos += count;
	return count;
}

/******************************************************
 * Misc support
 ******************************************************/
// Callbacks:  (structure defined in /linux/fs.h)
struct file_operations my_fops = {
	.owner    =  THIS_MODULE,
	.read     =  my_read,
	.write    =  my_write,
};

// Character Device info for the Kernel:
static struct miscdevice my_miscdevice = {
		.minor    = MISC_DYNAMIC_MINOR,         // Let the system assign one.
		.name     = MY_DEVICE_FILE,             // /dev/.... file.
		.fops     = &my_fops                    // Callback functions.
};


/******************************************************
 * Driver initialization and exit:
 ******************************************************/
static int __init my_init(void)
{
	int ret;
	printk(KERN_INFO "----> demo_misc driver init(): file /dev/%s.\n", MY_DEVICE_FILE);

	// Register as a misc driver:
	ret = misc_register(&my_miscdevice);

	initialize_data();

	return ret;
}

static void __exit my_exit(void)
{
	printk(KERN_INFO "<---- demo_misc driver exit().\n");

	// Unregister misc driver
	misc_deregister(&my_miscdevice);
}

module_init(my_init);
module_exit(my_exit);

MODULE_AUTHOR("Brian Fraser");
MODULE_DESCRIPTION("Demo driver to return some ascii data, and reverse a string to printk.");
MODULE_LICENSE("GPL");
