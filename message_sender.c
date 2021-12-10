
#include "message_slot.h"
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    unsigned long cid = (unsigned long) atoi(argv[2]);
    int fd;
    char *msg = argv[3];
    if (argc != 4) {
        perror("exactly 3 arguments are required");
        exit(1);
    }
    fd = open(argv[1], O_WRONLY);
    if (fd < 0) {
        perror("couldn't process your request 1");
        exit(1);
    }
    if (ioctl(fd, MSG_SLOT_CHANNEL, cid) < 0) {
        perror("couldn't process your request 2");
        exit(1);
    }
    if (write(fd, msg, strlen(msg)) < 0) {
        perror("couldn't send the message 3");
        exit(1);
    }
    if(close(fd) < 0){
        perror("couldn't process your request 4");
        exit(1);
    }
}