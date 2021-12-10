#ifndef _MESSAGE_SLOT_H
#define _MESSAGE_SLOT_H
#include <linux/ioctl.h>

#define MSG_SLOT_CHANNEL _IOW(240, 0, unsigned int)
#define BUF_LEN 128
#endif