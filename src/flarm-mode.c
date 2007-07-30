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

#include "flarm-internal.h"

#define FLARM_COMMAND_BINARY "$PFLAX\n"

flarm_result_t
flarm_enter_binary_mode(flarm_t flarm)
{
    flarm_result_t result;

    if (flarm->binary_mode)
        return FLARM_RESULT_NOT_TEXT;

    result = flarm_send(flarm, FLARM_COMMAND_BINARY,
                        sizeof(FLARM_COMMAND_BINARY) - 1);
    if (result == FLARM_RESULT_SUCCESS)
        flarm->binary_mode = 1;

    return result;
}

