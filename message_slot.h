#ifndef _MESSAGE_SLOT_H
#define _MESSAGE_SLOT_H
#include <linux/ioctl>

#define MAJOR 240
#define MSG_SLOT_CHANNEL _IOWR(MAJOR, 0, 0)
#define BUF_LEN 128