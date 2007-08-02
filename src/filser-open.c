/*
 * loggertools
 * Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
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

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <stdlib.h>

#include "filser.h"

int filser_fdopen(int fd, filser_t *device_r) {
    filser_t device;

    assert(device_r != NULL);
    assert(fd >= 0);

    device = calloc(1, sizeof(*device));
    if (device == NULL)
        return -1;

    device->fd = fd;

    *device_r = device;
    return 0;
}

int filser_open(const char *device_path, filser_t *device_r) {
    int fd, ret;
    struct termios attr;

    fd = open(device_path, O_RDWR | O_NOCTTY);
    if (fd < 0)
        return -1;

    ret = tcgetattr(fd, &attr);
    if (ret < 0) {
        int save_errno = errno;
        close(fd);
        errno = save_errno;
        return -1;
    }

    attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    attr.c_oflag &= ~OPOST;
    attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    attr.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | IXON | IXOFF);
    attr.c_cflag |= (CS8 | CLOCAL);
    attr.c_cc[VMIN] = 0;
    attr.c_cc[VTIME] = 1;
    cfsetospeed(&attr, B19200);
    cfsetispeed(&attr, B19200);

    ret = tcsetattr(fd, TCSANOW, &attr);
    if (ret < 0) {
        int save_errno = errno;
        close(fd);
        errno = save_errno;
        return -1;
    }

    ret = filser_fdopen(fd, device_r);
    if (ret != 0)
        close(fd);

    return ret;
}

void filser_close(filser_t *device_r) {
    filser_t device;

    assert(device_r != NULL);
    assert(*device_r != NULL);

    device = *device_r;
    *device_r = NULL;

    if (device->fd >= 0)
        close(device->fd);

    free(device);
}
