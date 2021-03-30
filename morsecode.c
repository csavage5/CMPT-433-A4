#include <linux/module.h>       
#include <linux/miscdevice.h>   //
#include <linux/kernel.h>       //
#include <linux/fs.h>           // 
#include <linux/leds.h>         // LED functions
#include <linux/uaccess.h>      // copy_to_user
#include <linux/slab.h>         // kmalloc
#include <linux/delay.h>        // msleep
#include <linux/kfifo.h>        // FIFO queue

// Solved VS Code IntelliSense issues with https://github.com/microsoft/vscode-cpptools/issues/5588

#define MY_DEVICE_FILE "morse-code"

#define DOT_TIME_NS 200000000
#define DOT_TIME_MS 200
DEFINE_LED_TRIGGER(my_trigger);

#define FIFO_SIZE 512
static DECLARE_KFIFO(ms_fifo, char, FIFO_SIZE);

enum bitType {NONE, DOT, PAUSE, SPACE};

char *pCharCursor;
int buffSize;

int buffIdx;
int ch;

short chMorseCode;
int i;

static int my_open(struct inode *inode, struct file *file);
static int my_close(struct inode *inode, struct file *file);
static ssize_t my_read(struct file *file, char *buff, size_t count, loff_t *ppos);
static ssize_t my_write(struct file *file, const char *buff, size_t count, loff_t *ppos);

static int __init morsecode_init(void);
static void __exit morsecode_exit(void);

static enum bitType lookAhead(short character); 
short getMorseCode(char letter);

void turnOnLED(void);
void turnOffLED(void);

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
    int numBytesRead = 0;

    if (kfifo_to_user(&ms_fifo, buff, count, &numBytesRead)) { 
        return -EFAULT;
    }
    
    return numBytesRead;
}

static ssize_t my_write(struct file *file, const char *buff, size_t count, loff_t *ppos) {

    char *copiedBuff = kmalloc(sizeof(char) * count, GFP_KERNEL);
    
    if (copy_from_user(copiedBuff, buff, sizeof(char) * count)) {
        return -EFAULT;
    }

    // remove leading and trailing whitespace and get new size
    pCharCursor = strim(copiedBuff);
    buffSize = strlen(pCharCursor);
    printk(KERN_INFO "stripped buffer size: %d\n", buffSize);


    for (buffIdx = 0; buffIdx < buffSize; buffIdx++) {
        
        ch = pCharCursor[buffIdx];

		if (ch < ' ' || (ch > ' ' && ch < 'A') || (ch > 'Z' && ch < 'a') || (ch > 'z')) {
            // CASE: character is unsupported, skip
            continue;
		}

        if (ch == ' ') {

            // CASE: break between words - add ' ' * 3 to FIFO queue
            if (!kfifo_put(&ms_fifo, ' ')) {
                // CASE: FIFO full
                return -EFAULT;
            }

            if (!kfifo_put(&ms_fifo, ' ')) {
                // CASE: FIFO full
                return -EFAULT;
            }

            if (!kfifo_put(&ms_fifo, ' ')) {
                // CASE: FIFO full
                return -EFAULT;
            }

            msleep(7 * DOT_TIME_MS);
            continue;
        }

		// get morse code for character      
        chMorseCode = getMorseCode(ch);

        // Output morse code for character
        for(i = 0; i < 16; i++) {
  
            if (chMorseCode & 0x8000 && lookAhead(chMorseCode) == DOT) {
                
                // CASE: current bit is 1 and the next bit is 
                //       also a 1 (DASH) - write '-' to FIFO queue
                if (!kfifo_put(&ms_fifo, '-')) {
                    // CASE: FIFO full
                    return -EFAULT;
                }

                turnOnLED();

                msleep(3 * DOT_TIME_MS);

                turnOffLED();

                // skip over rest of dash (next two 1s)
                chMorseCode <<= 2;

            } else if (chMorseCode & 0x8000) {
                // CASE: current bit is a 1 - add '.' to FIFO queue
                if (!kfifo_put(&ms_fifo, '.')) {
                    // CASE: FIFO full
                    return -EFAULT;
                }

                turnOnLED();
                msleep(DOT_TIME_MS);
                turnOffLED();

            } else {
                // CASE: bit is a 0 - either pause between dots or end of letter
                
                if (lookAhead(chMorseCode) == PAUSE) {
                    // CASE: found end of letter, stop reading
                    //       and go to next character
                    break;
                } 

                msleep(DOT_TIME_MS);

            }
            
            chMorseCode <<= 1;
        } // end of letter

        // wait between letters
        turnOffLED();
        if (buffIdx < buffSize - 1)  {
            // CASE: not on last letter 

            if (pCharCursor[buffIdx+1] != ' ') {
                // CASE: break between letters, push ' ' to FIFO and 
                //       wait for 3 DOT_TIME

                if (!kfifo_put(&ms_fifo, ' ')) {
                // CASE: FIFO full
                return -EFAULT;
                }

                msleep(3 * DOT_TIME_MS);  
            }

        }
        
	} // end of string

    // add '\n' to FIFO queue
    if (!kfifo_put(&ms_fifo, '\n')) {
        // CASE: FIFO full
        return -EFAULT;
    }

    kfree(copiedBuff);

    *ppos += count;
    return count;

}

// check the value of the bit at 2^14 (second MSB)
static enum bitType lookAhead(short morseCode) {
    if (morseCode & 0x4000) {
        return DOT;
    }
    return PAUSE;
}


static int __init morsecode_init(void) {
    int ret;
    ret = misc_register(&my_miscdevice);

    INIT_KFIFO(ms_fifo);

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

// convert char to morse code hex equivalent
short getMorseCode(char letter) {
    switch (letter) {
        case 'a':
        case 'A':
            return 0xB800;
        case 'b':
        case 'B':
            return 0xEA80;
        case 'c':
        case 'C':
            return 0xEBA0;
        case 'd':
        case 'D':
            return 0xEA00;
        case 'e':
        case 'E':
            return 0x8000;
        case 'f':
        case 'F':
            return 0xAE80;
        case 'g':
        case 'G':
            return 0xEE80;
        case 'h':
        case 'H':
            return 0xAA00;
        case 'i':
        case 'I':
            return 0xA000;
        case 'j':
        case 'J':
            return 0xBBB8;
        case 'k':
        case 'K':
            return 0xEB80;
        case 'l':
        case 'L':
            return 0xBA80;
        case 'm':
        case 'M':
            return 0xEE00;
        case 'n':
        case 'N':
            return 0xE800;
        case 'o':
        case 'O':
            return 0xEEE0;
        case 'p':
        case 'P':
            return 0xBBA0;
        case 'q':
        case 'Q':
            return 0xEEB8;
        case 'r':
        case 'R':
            return 0xBA00;
        case 's':
        case 'S':
            return 0xA800;
        case 't':
        case 'T':
            return 0xE000;
        case 'u':
        case 'U':
            return 0xAE00;
        case 'v':
        case 'V':
            return 0xAB80;
        case 'w':
        case 'W':
            return 0xBB80;
        case 'x':
        case 'X':
            return 0xEAE0;
        case 'y':
        case 'Y':
            return 0xEBB8;
        case 'z':
        case 'Z':
            return 0xEEA0;
        default:
            return 0x0000;
    }
}

void turnOnLED(void) {
    led_trigger_event(my_trigger, LED_FULL);
}

void turnOffLED(void) {
    led_trigger_event(my_trigger, LED_OFF);
}



// Information about this module:
MODULE_AUTHOR("Cameron Savage and Ryan Thompson");
MODULE_DESCRIPTION("Trigger for onboard LEDs that flashes in morse code"); 
MODULE_LICENSE("GPL"); // Important to leave as GPL.