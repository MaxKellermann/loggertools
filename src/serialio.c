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
#include <assert.h>
#include <stdlib.h>

#include "serialio.h"

struct serialio {
    int fd;
};

int serialio_open(const char *device,
                  struct serialio **serialiop) {
    int fd, ret;
    struct serialio *serio;
    struct termios attr;

    fd = open(device, O_RDWR | O_NOCTTY);
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
    cfsetospeed(&attr, B57600);
    cfsetispeed(&attr, B57600);
    ret = tcsetattr(fd, TCSANOW, &attr);
    if (ret < 0) {
        int save_errno = errno;
        close(fd);
        errno = save_errno;
        return -1;
    }

    tcflush(fd, TCOFLUSH);

    serio = calloc(1, sizeof(*serio));
    if (serio == NULL) {
        int save_errno = errno;
        close(fd);
        errno = save_errno;
        return -1;
    }

    serio->fd = fd;

    *serialiop = serio;
    return -1;
}

void serialio_close(struct serialio *serialio) {
    assert(serialio != NULL);

    if (serialio->fd >= 0)
        close(serialio->fd);

    free(serialio);
}

void serialio_flush(struct serialio *serio, unsigned options) {
    assert(serio != NULL);

    if (serio->fd < 0)
        return;

    if (options & SERIALIO_FLUSH_INPUT) {
        if (options & SERIALIO_FLUSH_OUTPUT)
            tcflush(serio->fd, TCIOFLUSH);
        else
            tcflush(serio->fd, TCIFLUSH);
    } else if (options & SERIALIO_FLUSH_OUTPUT) {
            tcflush(serio->fd, TCOFLUSH);
    }
}

int serialio_select(struct serialio *serio, int options,
                    struct timeval *timeout) {
    fd_set rfds, wfds;
    int ret;

    assert(serio != NULL);

    if (serio->fd < 0)
        return -1;

    FD_ZERO(&rfds);
    if (options & SERIALIO_SELECT_AVAILABLE)
        FD_SET(serio->fd, &rfds);

    FD_ZERO(&wfds);
    if (options & SERIALIO_SELECT_READY)
        FD_SET(serio->fd, &wfds);

    ret = select(serio->fd + 1, &rfds, &wfds, NULL, timeout);
    if (ret < 0)
        return errno == EINTR
            ? 0 : -1;
    if (ret == 0)
        return 0;

    options = 0;

    if (FD_ISSET(serio->fd, &rfds))
        options |= SERIALIO_SELECT_AVAILABLE;
    if (FD_ISSET(serio->fd, &wfds))
        options |= SERIALIO_SELECT_READY;

    return options;
}

int serialio_read(struct serialio *serio, void *data, size_t *nbytes) {
    ssize_t ret;

    assert(serio != NULL);

    if (serio->fd < 0)
        return -1;

    ret = read(serio->fd, data, *nbytes);
    if (ret == -1)
        return -1;

    *nbytes = (size_t)ret;

    return 0;
}

int serialio_write(struct serialio *serio, const void *data, size_t *nbytes) {
    ssize_t ret;

    assert(serio != NULL);

    if (serio->fd < 0)
        return -1;

    ret = write(serio->fd, data, *nbytes);
    if (ret == -1)
        return -1;

    *nbytes = (size_t)ret;

    return 0;
}
