/*
 * loggertools
 * Copyright (C) 2004-2008 Max Kellermann <max@duempel.org>
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
 */

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <termios.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/select.h>

#include "filser.h"

struct dump {
    const char *prefix;
    unsigned char line[0x10];
    size_t offset, column;
};

static void dump_eol(struct dump *d) {
    size_t i;

    if (d->column == 0)
        return;

    for (i = d->column; i < 0x10; i++)
        printf("   ");

    printf(" | ");

    for (i = 0; i < d->column; i++)
        printf("%c", d->line[i] >= 0x20 && d->line[i] < 0x80
               ? d->line[i] : '.');
    printf("\n");

    d->offset += d->column;
    d->column = 0;
}

static void dump_char(struct dump *d, const char *prefix,
                      unsigned char ch) {
    if (d->prefix != NULL && strcmp(prefix, d->prefix) != 0) {
        dump_eol(d);
        d->prefix = NULL;
    }

    if (d->prefix == NULL) {
        d->prefix = prefix;
        d->offset = 0;
        d->column = 0;
    }

    if (d->column == 0)
        printf("%s 0x%04x: ", prefix, (unsigned)d->offset);

    printf(" %02x", ch);
    fflush(stdout);

    d->line[d->column++] = ch;
    if (d->column >= 0x10)
        dump_eol(d);
}

static void syn_ack_wait(filser_t device) {
    int ret;
    unsigned tries = 20;

    do {
        alarm(10);
        ret = filser_syn_ack(device);
        alarm(0);
        if (ret < 0) {
            fprintf(stderr, "failed to connect: %s\n",
                    strerror(errno));
            _exit(1);
        }

        if (ret == 0) {
            fprintf(stderr, "no filser found, trying again\n");
            sleep(1);
        }
    } while (ret == 0 && tries-- > 0);

    if (ret == 0) {
        fprintf(stderr, "no filser\n");
        _exit(1);
    }
}

static int open_virtual(void) {
    int fd, ret;

    fd = open("/dev/ptmx", O_RDWR|O_NOCTTY);
    if (fd < 0) {
        fprintf(stderr, "failed to open /dev/ptmx: %s\n", strerror(errno));
        _exit(1);
    }

    ret = unlockpt(fd);
    if (ret < 0) {
        fprintf(stderr, "failed to unlockpt(): %s\n", strerror(errno));
        _exit(1);
    }

    printf("Slave terminal is %s\n", ptsname(fd));

    unlink("/tmp/fwd");
    ret = symlink(ptsname(fd), "/tmp/fwd");
    if (ret < 0) {
        fprintf(stderr, "symlink() failed: %s\n", strerror(errno));
        _exit(1);
    }

    printf("Symlinked to /tmp/fwd\n");

    return fd;
}

static void open_real(filser_t *device_r) {
    int ret;
    filser_t device;

    ret = filser_open("/dev/ttyS0.orig", &device);
    if (ret != 0) {
        fprintf(stderr, "failed to open ttyS0: %s\n",
                strerror(errno));
        _exit(1);
    }

    syn_ack_wait(device);

    *device_r = device;
}

int main(int argc, char **argv) {
    int virtual_fd, ret;
    filser_t real_device = NULL;
    fd_set rfds;
    struct dump dump;

    (void)argc;
    (void)argv;

    memset(&dump, 0, sizeof(dump));

    virtual_fd = open_virtual();

    while (1) {
        int max_fd;
        unsigned char data[16];
        ssize_t nbytes, i;

        max_fd = real_device == NULL || virtual_fd > real_device->fd
            ? virtual_fd : real_device->fd;

        FD_ZERO(&rfds);
        FD_SET(virtual_fd, &rfds);
        if (real_device != NULL)
            FD_SET(real_device->fd, &rfds);

        ret = select(max_fd + 1, &rfds, NULL, NULL, NULL);
        if (ret < 0) {
            fprintf(stderr, "select failed: %s\n", strerror(errno));
            _exit(1);
        }

        if (real_device != NULL && FD_ISSET(real_device->fd, &rfds)) {
            nbytes = read(real_device->fd, &data, sizeof(data));
            if (nbytes < 0) {
                fprintf(stderr, "read failed: %s\n", strerror(errno));
                _exit(1);
            }

            if (nbytes > 0) {
                for (i = 0; i < nbytes; i++)
                    dump_char(&dump, "RECV", data[i]);

                nbytes = write(virtual_fd, &data, (size_t)nbytes);
                if (nbytes < 0) {
                    fprintf(stderr, "write failed: %s\n", strerror(errno));
                    _exit(1);
                }
            }
        }

        if (FD_ISSET(virtual_fd, &rfds)) {
            nbytes = read(virtual_fd, &data, sizeof(data));
            if (nbytes < 0) {
                if (errno == EIO) {
                    close(virtual_fd);
                    filser_close(&real_device);
                    virtual_fd = open_virtual();
                    continue;
                }

                fprintf(stderr, "read failed: %s\n", strerror(errno));
                _exit(1);
            }

            if (nbytes > 0) {
                for (i = 0; i < nbytes; i++)
                    dump_char(&dump, "SEND", data[i]);

                if (real_device == NULL)
                    open_real(&real_device);

                if (nbytes == 1 && data[0] == 0x16)
                    tcflush(real_device->fd, TCIOFLUSH);

                nbytes = write(real_device->fd, &data, (size_t)nbytes);
                if (nbytes < 0) {
                    fprintf(stderr, "write failed: %s\n", strerror(errno));
                    _exit(1);
                }
            }
        }
    }

    return 0;
}
