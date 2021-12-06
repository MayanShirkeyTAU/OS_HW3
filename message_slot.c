#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/errno.h>

MODULE_LICENSE("GPL");

#define SUCCESS 0
#define EXISTS -1
#define DEVICE_RANGE_NAME "char_dev"
#define BUF_LEN 128
#define DEVICE_FILE_NAME "simple_char_dev"
#define EMPTY '\0'

// device MAJOR number
static const int MAJOR;

struct chardev_info {
    spinlock_t lock;
};

typedef struct channel {
    char buffer[BUF_LEN];
    int channel_id;
    struct channel *next;
} channel;

typedef struct msg_slot {
    int minor;
    channel *first_channel, *active_channel;
    struct msg_slot *next;
} msg_slot;

msg_slot *first_slot = NULL;

// used to prevent concurent access into the same device
static int dev_open_flag = 0;

static struct chardev_info device_info;

static void init_msg_slot(msg_slot *slot, int minor) {
    slot->minor = minor;
    slot->first_channel = NULL;
    slot->active_channel = NULL;
    slot->next = NULL;
}

static void init_channel(channel *c, int channel_id) {
    int i;
    c->channel_id = channel_id;
    for (i = 0; i < BUF_LEN; i++) c->buffer[i] = EMPTY;
    c->next = NULL;
}

static void activate_channel(msg_slot *slot, unsigned long channel_id) {
    channel *c = slot->first_channel;
    if (slot->first_channel == NULL) {
        slot->first_channel = (channel *) kmalloc(sizeof(channel), GFP_KERNEL);
        init_channel(slot->first_channel, channel_id);
        slot->active_channel = slot->first_channel;
    }
    while (c->channel_id != channel_id && c->next != NULL) c = c->next;
    if (c->channel_id == channel_id) slot->active_channel = c;
    else {
        c->next = (channel *) kmalloc(sizeof(channel), GFP_KERNEL);
        init_channel(c->next, channel_id);
        slot->active_channel = c->next;
    }
}

//================== DEVICE FUNCTIONS ===========================
static long device_ioctl(struct file *file,
                         unsigned int ioctl_command_id,
                         unsigned long ioctl_param) {
    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0) return -EINVAL;
    msg_slot *slot = (msg_slot *) file->private_data;
    activate_channel(slot, ioctl_param);
}


static int device_open(struct inode *inode,
                       struct file *file) {
    int minor;
    msg_slot *current;

    minor = iminor(inode);
    current = first_slot;
    if (first_slot == NULL) {
        first_slot = (msg_slot *) kmalloc(sizeof(msg_slot), GFP_KERNEL);
        init_msg_slot(first_slot, minor);
        file->private_data = (void *) first_slot;
    } else {
        while ((current->next != NULL) && (current->minor != minor)) current = current->next;
        if (current->minor == minor) {
            current->next = (msg_slot *) kmalloc(sizeof(msg_slot), GFP_KERNEL);
            init_msg_slot(current->next, minor);
            file->private_data = (void *) current->next;
        }
    }
    return SUCCESS;
}

//---------------------------------------------------------------
static int device_release(struct inode *inode, struct file *file) {
    return 0;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset) {
    char receiver[length];
    int i = -1;
    if (slot->active_channel == NULL) return -EINVAL;
    msg_slot *slot = (msg_slot *) file->private_data;
    while (++i < length) {
        if (slot->active_channel->buffer[i] == EMPTY) return -EWOULDBLOCK;
        if (put_user(slot->active_channel->buffer[i], receiver[i]) < 0) return -EFAULT;
    }
    slot->active_channel->buffer = receiver;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
    int i = -1;
    char receiver[length];
    msg_slot *slot = (msg_slot *) file->private_data;
    if (slot->active_channel == NULL) return -EINVAL;
    if (length < 0 || length > BUF_LEN) return -EMSGSIZE;
    while (++i < length)
        if (get_user(receiver[i], buffer[i]) < 0) return -EFAULT;
    slot->active_channel->buffer = receiver;
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