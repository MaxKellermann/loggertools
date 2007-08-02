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

#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "filser.h"

int filser_send_syn(filser_t device) {
    static const unsigned char syn = FILSER_SYN;
    ssize_t nbytes;

    tcflush(device->fd, TCIOFLUSH);

    nbytes = write(device->fd, &syn, sizeof(syn));
    if (nbytes <= 0)
        return (int)nbytes;

    return 1;
}

int filser_recv_ack(filser_t device) {
    static const unsigned char ack = FILSER_ACK;
    unsigned char buffer;
    ssize_t nbytes;

    nbytes = read(device->fd, &buffer, sizeof(buffer));
    if (nbytes <= 0)
        return (int)nbytes;

    return buffer == ack
        ? 1 : 0;
}

int filser_syn_ack(filser_t device) {
    int ret;

    ret = filser_send_syn(device);
    if (ret <= 0)
        return ret;

    return filser_recv_ack(device);
}

int filser_send_command(filser_t device, unsigned char cmd) {
    const unsigned char buffer[2] = { FILSER_PREFIX, cmd };
    ssize_t nbytes;

    tcflush(device->fd, TCIOFLUSH);

    nbytes = write(device->fd, buffer, sizeof(buffer));
    if (nbytes <= 0)
        return (int)nbytes;

    if (nbytes != sizeof(buffer))
        return 0;

    return 1;
}
