/*
 * loggertools
 * Copyright (C) 2004 Max Kellermann (max@duempel.org)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the License
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * $Id$
 */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <time.h>

static void alarm_handler(int dummy) {
    (void)dummy;
}

static ssize_t wait_for_prompt(int fd, char *buffer, size_t buffer_size) {
    ssize_t nbytes;
    unsigned z;
    size_t buffer_pos = 0;
    time_t timeout = time(NULL) + 20;

    alarm(20);

    do {
        if (buffer_pos >= buffer_size)
            buffer_pos = 0;

        nbytes = read(fd, buffer + buffer_pos, buffer_size - buffer_pos);
        for (z = 0; z < (unsigned)nbytes; z++) {
            fprintf(stderr, "%c", buffer[buffer_pos + z]);
            fflush(stderr);
        }
        if (nbytes < 0) {
            alarm(0);
            tcflush(fd, TCIFLUSH);
            return errno == EINTR
                ? 0 : -1;
        }

        if (memchr(buffer + buffer_pos, ':', nbytes) != NULL) {
            alarm(0);
            buffer_pos += (size_t)nbytes;
            return (ssize_t)buffer_pos;
        }

        buffer_pos += (size_t)nbytes;
    } while (time(NULL) <= timeout);

    return 0;
}

static int wait_for_asterisk(int fd) {
    char buffer[256];
    ssize_t nbytes;
    time_t timeout = time(NULL) + 5;

    alarm(5);

    do {
        nbytes = read(fd, buffer, sizeof(buffer));
        if (nbytes < 0) {
            alarm(0);
            tcflush(fd, TCIFLUSH);
            return errno == EINTR
                ? 0 : -1;
        }

        if (memchr(buffer, '*', nbytes) != NULL) {
            alarm(0);
            tcflush(fd, TCIFLUSH);
            return 1;
        }
    } while (time(NULL) <= timeout);

    return 0;
}

static void do_menu(int fd, char p_or_d) {
    char buffer[1024];
    ssize_t nbytes;
    char ch;

    tcflush(fd, TCOFLUSH);

    for (;;) {
        nbytes = wait_for_prompt(fd, buffer, sizeof(buffer) - 1);
        if (nbytes < 0) {
            fprintf(stderr, "failed to read: %s\n",
                    strerror(errno));
            _exit(1);
        }

        if (nbytes == 0) {
            fprintf(stderr, "timeout, no prompt\n");
            _exit(1);
        }

        buffer[nbytes] = 0;

        if (strstr(buffer, "Y to confirm") != NULL ||
            strstr(buffer, "\"Y\" to confirm") != NULL ||
            strstr(buffer, "\"Y\" to continue") != NULL) {
            ch = 'Y';
            write(fd, &ch, sizeof(ch));
        } else if (strstr(buffer, "Program [P]") != NULL) {
            write(fd, &p_or_d, sizeof(p_or_d));
        } else if (strstr(buffer, "waiting for") != NULL) {
            return;
        } else {
            fprintf(stderr, "unknown prompt\n");
            _exit(1);
        }
    }
}

int main(int argc, char **argv) {
    const char *filename = NULL;
    const char *device = "/dev/ttyS0";
    FILE *file;
    int fd, ret;
    char line[256];
    size_t length;
    ssize_t nbytes;
    struct termios attr;

    signal(SIGALRM, alarm_handler);

    if (argc != 3) {
        fprintf(stderr, "usage: cenfistool send filename.bhf\n");
        _exit(1);
    }

    if (strcmp(argv[1], "send") != 0) {
        fprintf(stderr, "usage: cenfistool send filename.bhf\n");
        _exit(1);
    }

    filename = argv[2];

    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "failed to open '%s': %s\n",
                filename, strerror(errno));
        _exit(1);
    }

    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "failed to open '%s': %s\n",
                device, strerror(errno));
        _exit(1);
    }

    ret = tcgetattr(fd, &attr);
    if (ret < 0) {
        fprintf(stderr, "tcgetattr failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    attr.c_oflag &= ~OPOST;
    attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    attr.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | IXON | IXOFF);
    attr.c_cflag |= (CS8 | CLOCAL);
    attr.c_cc[VMIN] = 0;
    attr.c_cc[VTIME] = 1;
    cfsetospeed(&attr, B57600);
    cfsetispeed(&attr, B57600);
    ret = tcsetattr(fd, TCSANOW, &attr);
    if (ret < 0) {
        fprintf(stderr, "tcsetattr failed: %s\n",
                strerror(errno));
        _exit(1);
    }

    tcflush(fd, TCOFLUSH);

    do_menu(fd, 'P');

    while (fgets(line, sizeof(line) - 2, file) != NULL) {
        length = strlen(line);
        while (length > 0 && line[length - 1] >= 0 &&
               line[length - 1] <= ' ')
            length--;

        if (length == 0)
            continue;

        line[length++] = '\r';
        line[length++] = '\n';

        nbytes = write(fd, line, length);
        if (nbytes < 0) {
            fprintf(stderr, "failed to write to '%s': %s\n",
                    device, strerror(errno));
            _exit(1);
        }

        if ((size_t)nbytes < length) {
            fprintf(stderr, "short write to '%s'\n",
                    device);
            _exit(1);
        }

        ret = wait_for_asterisk(fd);
        if (ret < 0) {
            fprintf(stderr, "read from '%s' failed: %s\n",
                    device, strerror(errno));
            _exit(1);
        }

        if (ret == 0) {
            fprintf(stderr, "timeout on '%s'\n",
                    device);
            _exit(1);
        }
    }

    return 0;
}
