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

#include "cenfis.h"

struct cenfis {
    int fd, dump_fd;
    cenfis_status_t status;
    unsigned buffer_pos;
    char buffer[2048];
};

cenfis_status_t cenfis_open(const char *device,
                            struct cenfis **cenfisp) {
    int fd, ret;
    struct cenfis *cenfis;
    struct termios attr;

    fd = open(device, O_RDWR | O_NOCTTY);
    if (fd < 0)
        return CENFIS_STATUS_ERRNO;

    ret = tcgetattr(fd, &attr);
    if (ret < 0) {
        int save_errno = errno;
        close(fd);
        errno = save_errno;
        return CENFIS_STATUS_ERRNO;
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
        return CENFIS_STATUS_ERRNO;
    }

    tcflush(fd, TCOFLUSH);

    cenfis = calloc(1, sizeof(*cenfis));
    if (cenfis == NULL) {
        int save_errno = errno;
        close(fd);
        errno = save_errno;
        return CENFIS_STATUS_ERRNO;
    }

    cenfis->fd = fd;
    cenfis->dump_fd = -1;
    cenfis->status = CENFIS_STATUS_IDLE;

    *cenfisp = cenfis;
    return CENFIS_STATUS_SUCCESS;
}

void cenfis_close(struct cenfis *cenfis) {
    assert(cenfis != NULL);

    if (cenfis->fd >= 0)
        close(cenfis->fd);

    free(cenfis);
}

void cenfis_dump(struct cenfis *cenfis, int fd) {
    cenfis->dump_fd = fd;
}

static void cenfis_invalidate(struct cenfis *cenfis) {
    int save_errno = errno;

    assert(cenfis != NULL);
    assert(cenfis->fd >= 0);
    assert(!cenfis_is_error(cenfis->status));

    close(cenfis->fd);
    cenfis->fd = -1;
    cenfis->status = CENFIS_STATUS_INVALID;

    errno = save_errno;
}

static void cenfis_flush(struct cenfis *cenfis) {
    assert(cenfis != NULL);
    assert(cenfis->fd >= 0);
    assert(!cenfis_is_error(cenfis->status));

    tcflush(cenfis->fd, TCIFLUSH);
    cenfis->buffer_pos = 0;
}

cenfis_status_t cenfis_select(struct cenfis *cenfis,
                              struct timeval *timeout) {
    int ret;
    fd_set rfds;
    ssize_t nbytes;
    char *p;

    if (cenfis_is_error(cenfis->status) || timeout == NULL)
        return cenfis->status;

    assert(timeout->tv_sec > 0 || timeout->tv_usec > 0);

    do {
        /* select on file handle */
        FD_ZERO(&rfds);
        FD_SET(cenfis->fd, &rfds);

        ret = select(cenfis->fd + 1, &rfds, NULL, NULL, timeout);
        if (ret < 0) {
            if (errno == EINTR)
                return CENFIS_STATUS_ERRNO;

            cenfis_invalidate(cenfis);
            return CENFIS_STATUS_ERRNO;
        }

        if (ret == 0)
            break;

        /* read from file handle */
        if (cenfis->buffer_pos >= sizeof(cenfis->buffer) - sizeof(cenfis->buffer) / 4) {
            /* free a part of the buffer */
            memmove(cenfis->buffer, cenfis->buffer + sizeof(cenfis->buffer) / 4,
                    (sizeof(cenfis->buffer) * 3) / 4);
            cenfis->buffer_pos -= sizeof(cenfis->buffer) / 4;
        }

        nbytes = read(cenfis->fd, cenfis->buffer + cenfis->buffer_pos,
                      sizeof(cenfis->buffer) - cenfis->buffer_pos);
        if (nbytes < 0) {
            int save_errno = errno;

            if (save_errno == EINTR)
                return CENFIS_STATUS_ERRNO;

            cenfis_invalidate(cenfis);
            return CENFIS_STATUS_ERRNO;
        }

        write(cenfis->dump_fd, cenfis->buffer + cenfis->buffer_pos,
              (size_t)nbytes);

        /* check what's there */
        switch (cenfis->status) {
        case CENFIS_STATUS_SHORT_WRITE:
        case CENFIS_STATUS_CONTEXT:
        case CENFIS_STATUS_INVALID:
        case CENFIS_STATUS_ERRNO:
            /* suppress warning */
            break;

        case CENFIS_STATUS_WAIT_ACK:
            if (memchr(cenfis->buffer + cenfis->buffer_pos,
                       '*', (size_t)nbytes) != NULL) {
                cenfis_flush(cenfis);
                return cenfis->status = CENFIS_STATUS_DATA;
            }
            break;

        case CENFIS_STATUS_SUCCESS:
        case CENFIS_STATUS_IDLE:
            p = memchr(cenfis->buffer + cenfis->buffer_pos,
                       ':', (size_t)nbytes);
            if (p != NULL) {
                *p = 0;

                while (p > cenfis->buffer && (unsigned char)p[-1] >= ' ')
                    p--;

                if (strstr(p, "Y to confirm") != NULL ||
                    strstr(p, "\"Y\" to confirm") != NULL ||
                    strstr(p, "\"Y\" to continue") != NULL) {
                    cenfis_flush(cenfis);
                    return cenfis->status = CENFIS_STATUS_DIALOG_CONFIRM;
                } else if (strstr(p, "Program [P]") != NULL) {
                    cenfis_flush(cenfis);
                    return cenfis->status = CENFIS_STATUS_DIALOG_SELECT;
                } else if (strstr(p, "waiting for") != NULL) {
                    cenfis_flush(cenfis);
                    return cenfis->status = CENFIS_STATUS_DATA;
                }
            }

            cenfis->buffer_pos += (unsigned)nbytes;
            assert(cenfis->buffer_pos <= sizeof(cenfis->buffer));
            break;

        case CENFIS_STATUS_DIALOG_CONFIRM:
        case CENFIS_STATUS_DIALOG_SELECT:
        case CENFIS_STATUS_DATA:
            /* we are expected to send, ignore what cenfis says
               meanwhile */
            cenfis_flush(cenfis);
            break;
        }
    } while (timeout->tv_sec > 0 || timeout->tv_usec > 0);

    return cenfis->status;
}

static cenfis_status_t cenfis_write(struct cenfis *cenfis,
                                    const void *buf, size_t count) {
    ssize_t nbytes;

    assert(cenfis != NULL);
    assert(!cenfis_is_error(cenfis->status));

    nbytes = write(cenfis->fd, buf, count);
    if (nbytes < 0) {
        cenfis_invalidate(cenfis);
        return CENFIS_STATUS_ERRNO;
    }

    if ((size_t)nbytes != count) {
        cenfis_invalidate(cenfis);
        return CENFIS_STATUS_SHORT_WRITE;
    }

    return CENFIS_STATUS_SUCCESS;
}

cenfis_status_t cenfis_confirm(struct cenfis *cenfis) {
    static const char yes = 'Y';
    cenfis_status_t status;

    if (cenfis_is_error(cenfis->status))
        return cenfis->status;

    if (cenfis->status != CENFIS_STATUS_DIALOG_CONFIRM)
        return CENFIS_STATUS_CONTEXT;

    status = cenfis_write(cenfis, &yes, sizeof(yes));
    if (status == CENFIS_STATUS_SUCCESS)
        cenfis->status = CENFIS_STATUS_IDLE;

    return status;
}

cenfis_status_t cenfis_dialog_respond(struct cenfis *cenfis, char ch) {
    cenfis_status_t status;

    if (cenfis_is_error(cenfis->status))
        return cenfis->status;

    if (cenfis->status != CENFIS_STATUS_DIALOG_SELECT)
        return CENFIS_STATUS_CONTEXT;

    status = cenfis_write(cenfis, &ch, sizeof(ch));
    if (status == CENFIS_STATUS_SUCCESS)
        cenfis->status = CENFIS_STATUS_IDLE;

    return status;
}

cenfis_status_t cenfis_write_data(struct cenfis *cenfis,
                                  const void *buf, size_t count) {
    cenfis_status_t status;

    if (cenfis_is_error(cenfis->status))
        return cenfis->status;

    if (cenfis->status != CENFIS_STATUS_DATA)
        return CENFIS_STATUS_CONTEXT;

    status = cenfis_write(cenfis, buf, count);
    if (status == CENFIS_STATUS_SUCCESS)
        cenfis->status = CENFIS_STATUS_WAIT_ACK;

    return status;
}
