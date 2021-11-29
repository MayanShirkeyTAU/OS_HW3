#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/malloc.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");

#define SUCCESS 0
#define EXISTS -1
#define DEVICE_RANGE_NAME "char_dev"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "simple_char_dev"

// device MAJOR number
static const int MAJOR;

struct chardev_info {
    spinlock_t lock;
};

typedef struct slot {
    int minor;
    struct slot *next;
} msgSlot;

msgSlot *first_slot = NULL;

// used to prevent concurent access into the same device
static int dev_open_flag = 0;

static struct chardev_info device_info;

// The message the device will give when asked
static char the_message[BUF_LEN];

//================== DEVICE FUNCTIONS ===========================
static int device_open(struct inode *inode,
                       struct file *file) {
    unsigned long flags; // for spinlock
    int minor;
    msgSlot *current;

    // We don't want to talk to two processes at the same time
    spin_lock_irqsave(&device_info.lock, flags);
    if (dev_open_flag) {
        spin_unlock_irqrestore(&device_info.lock, flags);
        return -EBUSY;
    }
    minor = iminor(inode);
    if (first_slot == NULL) {
        first_slot = (msgSlot *) valloc(sizeof(msgSlot));
        first_slot->minor = minor;
        first_slot->next = NULL;
    } else {
        current = first_slot;
        while ((current->next != NULL) && (current->minor != minor)) current = current->next;
        if (current->next != NULL) return EXISTS;
    }
    dev_open_flag++;

    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode,
                          struct file *file) {
    unsigned long flags; // for spinlock
    printk("Invoking device_release(%p,%p)\n", inode, file);

    // ready for our next caller
    spin_lock_irqsave(&device_info.lock, flags);
    dev_open_flag--;
    spin_unlock_irqrestore(&device_info.lock, flags);
    return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read(struct file *file,
                           char __user

* buffer,
size_t length,
        loff_t
*      offset )
{
// read doesnt really do anything (for now)
printk( "Invocing device_read(%p,%ld) - "
"operation not supported yet\n"
"(last written - %s)\n",
file, length, the_message );
//invalid argument error
return -
EINVAL;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file *file,
                            const char __user

* buffer,
size_t length,
        loff_t
*            offset)
{
int i;
printk("Invoking device_write(%p,%ld)\n", file, length);
for(
i = 0;
i < length && i < BUF_LEN; ++i )
{
get_user(the_message[i],
&buffer[i]);
}

// return the number of input characters used
return
i;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .owner      = THIS_MODULE, // Required for correct count of module usage. This prevents the module from being removed while used.
                .read           = device_read,
                .write          = device_write,
                .open           = device_open,
                .release        = device_release,
        };

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init

simple_init(void) {
    // init dev struct
    memset(&device_info, 0, sizeof(struct chardev_info));
    spin_lock_init(&device_info.lock);

    // Register driver capabilities. Obtain MAJOR num
    MAJOR = register_chrdev(0, DEVICE_RANGE_NAME, &Fops);

    // Negative values signify an error
    if (MAJOR < 0) {
        printk(KERN_ALERT
        "%s registraion failed for  %d\n",
                DEVICE_FILE_NAME, MAJOR );
        return MAJOR;
    }

    printk("Registeration is successful. "
           "The MAJOR device number is %d.\n", MAJOR);
    printk("If you want to talk to the device driver,\n");
    printk("you have to create a device file:\n");
    printk("mknod /dev/%s c %d 0\n", DEVICE_FILE_NAME, MAJOR);
    printk("You can echo/cat to/from the device file.\n");
    printk("Dont forget to rm the device file and "
           "rmmod when you're done\n");

    return 0;
}

//---------------------------------------------------------------
static void __exit

simple_cleanup(void) {
    // Unregister the device
    // Should always succeed
    unregister_chrdev(MAJOR, DEVICE_RANGE_NAME);
}

//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================