#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include "message_slot.h"
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

#define SUCCESS 0
#define DEVICE_RANGE_NAME "message_slot"
#define EMPTY '\0'

// device MAJOR number
static const int MAJOR = 240;

typedef struct channel {
    char buffer[BUF_LEN];
    ssize_t len;
    unsigned int id;
    struct channel *next;
} channel;

typedef struct msg_slot {
    int minor;
    channel *first_channel, *active_channel;
    struct msg_slot *next;
} msg_slot;

msg_slot *first_slot = NULL;

static void init_channel(channel *c, unsigned int cid) {
    c->id = cid;
    c->len = 0;
    c->next = NULL;
}

static channel * free_channel(channel *c){
    channel *next;
    next = c->next;
    if (c == NULL) return NULL;
    printk("\t\t\tFREEING CHANNEL %d\n", c->id);
    kfree(c);
    return next;
}

static msg_slot * free_msg_slot(msg_slot *slot){
    msg_slot *next;
    channel *c;
    int i;
    if (slot == NULL) return NULL;
    printk("FREEING SLOT %d AND ITS CHANNELS:\n", slot->minor);
    next = slot->next;
    c = slot->first_channel;
    for (i = 0; i < 256 && c != NULL; i++) c = free_channel(c);
    kfree(slot);
    return next;
}

 static msg_slot * init_msg_slot(int minor) {
    msg_slot *slot = (msg_slot *) kmalloc(sizeof(msg_slot), GFP_KERNEL);
    slot->minor = minor;
    slot->first_channel = NULL;
    slot->active_channel = NULL;
    slot->next = NULL;
    return slot;
}

static void activate_channel(msg_slot *slot, unsigned int cid) {
    channel *c = slot->first_channel;
    if (slot->first_channel == NULL) {
        slot->first_channel = (channel *) kmalloc(sizeof(channel), GFP_KERNEL);
        init_channel(slot->first_channel, cid);
        slot->active_channel = slot->first_channel;
    }
    else {
        while (c->id != cid && c->next != NULL) c = c->next;
        if (c->id == cid) slot->active_channel = c;
        else {
            c->next = (channel *) kmalloc(sizeof(channel), GFP_KERNEL);
            init_channel(c->next, cid);
            slot->active_channel = c->next;
        }
    }
}

//================== DEVICE FUNCTIONS ===========================
static long device_ioctl(struct file *file, unsigned int ioctl_command_id, unsigned long ioctl_param) {
    if (ioctl_command_id != MSG_SLOT_CHANNEL || ioctl_param == 0)
        return -EINVAL;
    activate_channel((msg_slot *) file->private_data, ioctl_param);
    return SUCCESS;
}


static int device_open(struct inode *inode, struct file *file) {
    msg_slot *curr;
    unsigned int minor;
    minor = iminor(inode);
    curr = first_slot;
    if (curr == NULL) {
        file->private_data = (void *) (first_slot = init_msg_slot(minor));
        return SUCCESS;
    }
    while ((curr->next != NULL) && (curr->minor != minor)) curr = curr->next;
    if (curr->minor == minor) file->private_data = (void *) curr;
    else file->private_data = (void *) (curr->next = init_msg_slot(minor));
    return SUCCESS;
}

static int device_release(struct inode *inode, struct file *file) {
    return SUCCESS;
}

static ssize_t device_read(struct file *file, char __user *buffer, size_t length, loff_t *offset) {
    int i;
    msg_slot *slot;
    slot = (msg_slot *) file->private_data;
    if (slot->active_channel == NULL) return -EINVAL;
    if(slot->active_channel->len > length) return -ENOSPC;
    if(slot->active_channel->len == 0) return -EWOULDBLOCK;
    for (i = 0; i < slot->active_channel->len; i++)
        if (put_user(slot->active_channel->buffer[i], &buffer[i]) < 0) return -EFAULT;
    return slot->active_channel->len;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write(struct file *file, const char __user *buffer, size_t length, loff_t *offset) {
    int i;
    char *receiver = (char *) kmalloc(sizeof(char) * length, GFP_KERNEL);
    msg_slot *slot;
    slot = (msg_slot *) file->private_data;
    if (slot->active_channel == NULL) return -EINVAL;
    if (length < 0 || length > BUF_LEN) return -EMSGSIZE;
    slot->active_channel->len = 0;
    for (i = 0; i < length; i++)
        if (get_user(receiver[i], &buffer[i]) < 0) return -EFAULT;
    for (i = 0; i < length; i++) {
        slot->active_channel->buffer[i] = receiver[i];
        slot->active_channel->len++;
    }
    return length;
}

//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
        {
                .owner          = THIS_MODULE,
                .open           = device_open,
                .unlocked_ioctl = device_ioctl,
                .read           = device_read,
                .write          = device_write,
                .release        = device_release
        };

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init device_init(void) {
    int result = register_chrdev(MAJOR, DEVICE_RANGE_NAME, &Fops);
    if (result < 0)
        return result;
    return 0;
}

static void __exit device_cleanup(void) {
    msg_slot *slot = first_slot;
    int i;
    unregister_chrdev(MAJOR, DEVICE_RANGE_NAME);
    printk("PERFORM EXIT");
    for (i = 0; i < 256 && slot != NULL; i++) slot = free_msg_slot(slot);
}

module_init(device_init);
module_exit(device_cleanup);
