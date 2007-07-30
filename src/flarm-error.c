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

#include "flarm.h"

#include <string.h>

const char *
flarm_strerror(flarm_result_t result)
{
    if (result > 0)
        return strerror(result);

    switch (result) {
    case FLARM_RESULT_SUCCESS:
        return "Success";

    case FLARM_RESULT_NOT_BINARY:
        return "Not in binary mode";

    case FLARM_RESULT_NOT_TEXT:
        return "Not in text mode";

    case FLARM_RESULT_NACK:
        return "Received NACK";

    case FLARM_RESULT_NOT_ACK:
        return "ACK expected";
    }

    return "Unknown FLARM library error";
}
