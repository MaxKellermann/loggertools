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

#include "flarm.h"
#include "flarm-internal.h"

flarm_result_t
flarm_fdopen(int fd, flarm_t *flarm_r) {
    flarm_t flarm;
    int ret;

    assert(flarm_r != NULL);
    assert(fd >= 0);

    flarm = calloc(1, sizeof(*flarm));
    if (flarm == NULL)
        return errno;

    flarm->fd = fd;

    ret = fifo_buffer_new(4096, &flarm->in);
    if (ret != 0) {
        flarm_close(&flarm);
        return ret;
    }

    ret = fifo_buffer_new(4096, &flarm->frame);
    if (ret != 0) {
        flarm_close(&flarm);
        return ret;
    }

    *flarm_r = flarm;
    return 0;
}

int flarm_open(const char *device_path, flarm_t *flarm_r) {
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
    cfsetospeed(&attr, B19200);
    cfsetispeed(&attr, B19200);

    ret = tcsetattr(fd, TCSANOW, &attr);
    if (ret < 0) {
        int save_errno = errno;
        close(fd);
        return save_errno;
    }

    ret = flarm_fdopen(fd, flarm_r);
    if (ret != FLARM_RESULT_SUCCESS)
        close(fd);

    return ret;
}

int flarm_fileno(flarm_t flarm) {
    assert(flarm != NULL);
    assert(flarm->fd >= 0);

    return flarm->fd;
}

void flarm_close(flarm_t *flarm_r) {
    flarm_t flarm;

    assert(flarm_r != NULL);
    assert(*flarm_r != NULL);

    flarm = *flarm_r;
    *flarm_r = NULL;

    if (flarm->fd >= 0)
        close(flarm->fd);

    if (flarm->in != NULL)
        fifo_buffer_delete(&flarm->in);

    if (flarm->frame != NULL)
        fifo_buffer_delete(&flarm->frame);

    free(flarm);
}
