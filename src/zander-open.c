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

#include "zander.h"
#include "zander-internal.h"

int zander_fdopen(int fd, zander_t *zander_r) {
    zander_t zander;

    assert(zander_r != NULL);
    assert(fd >= 0);

    zander = calloc(1, sizeof(*zander));
    if (zander == NULL)
        return -1;

    zander->fd = fd;

    *zander_r = zander;
    return 0;
}

int zander_open(const char *device_path, zander_t *zander_r) {
    int fd, ret;
    struct termios attr;

    fd = open(device_path, O_RDWR | O_NOCTTY);
    if (fd < 0)
        return errno;

    ret = tcgetattr(fd, &attr);
    if (ret < 0) {
        int save_errno = errno;
        close(fd);
        return save_errno;
    }

    attr.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    attr.c_oflag &= ~OPOST;
    attr.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    attr.c_cflag &= ~(CSIZE | PARENB | CRTSCTS | IXON | IXOFF);
    attr.c_cflag |= (CS8 | CLOCAL);
    attr.c_cc[VMIN] = 0;
    attr.c_cc[VTIME] = 1;
    cfsetospeed(&attr, B9600);
    cfsetispeed(&attr, B9600);

    ret = tcsetattr(fd, TCSANOW, &attr);
    if (ret < 0) {
        int save_errno = errno;
        close(fd);
        return save_errno;
    }

    ret = zander_fdopen(fd, zander_r);
    if (ret != 0)
        close(fd);

    return ret;
}

int zander_fileno(zander_t zander) {
    assert(zander != NULL);
    assert(zander->fd >= 0);

    return zander->fd;
}

void zander_close(zander_t *zander_r) {
    zander_t zander;

    assert(zander_r != NULL);
    assert(*zander_r != NULL);

    zander = *zander_r;
    *zander_r = NULL;

    if (zander->fd >= 0)
        close(zander->fd);

    free(zander);
}
