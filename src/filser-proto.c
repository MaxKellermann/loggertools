/*
 * loggertools
 * Copyright (C) 2004-2006 Max Kellermann (max@duempel.org)
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
#include <termios.h>
#include <unistd.h>

#include "filser.h"

int filser_syn_ack(int fd) {
    static const char syn = FILSER_SYN, ack = FILSER_ACK;
    char buffer;
    ssize_t nbytes;

    tcflush(fd, TCIOFLUSH);

    nbytes = write(fd, &syn, sizeof(syn));
    if (nbytes <= 0)
        return (int)nbytes;

    nbytes = read(fd, &buffer, sizeof(buffer));
    if (nbytes <= 0)
        return (int)nbytes;

    return buffer == ack
        ? 1 : 0;
}
