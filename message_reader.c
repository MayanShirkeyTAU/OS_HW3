
#include "message_slot.h"
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define PRINTLN 1

int main(int argc, char *argv[]) {
    unsigned long cid = (unsigned long) atoi(argv[2]);
    char buffer[BUF_LEN];
    int fd, msg_len;
    if (argc != 3) {
        perror("exactly 2 arguments are required.");
        exit(1);
    }
    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("couldn't process your request");
        exit(1);
    }
    if (ioctl(fd, MSG_SLOT_CHANNEL, cid) < 0) {
        perror("couldn't process your request");
        exit(1);
    }
    msg_len = read(fd, buffer, BUF_LEN);
    if (msg_len < 0) {
        perror("we couldn't read the message");
        exit(1);
    }
    if(write(PRINTLN, buffer, msg_len)){
        perror("we couldn't print your message");
        exit(1);
    }
    if(close(fd) < 0){
        perror("couldn't process your request");
        exit(1);
    }
}