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

#ifndef __ZANDER_IGC_H
#define __ZANDER_IGC_H

#include "zander.h"

#include <stdio.h>

enum zander_to_igc_result {
    ZANDER_IGC_SUCCESS,
    ZANDER_IGC_MALFORMED,
    ZANDER_IGC_ERRNO,
    ZANDER_IGC_EOF
};

enum zander_to_igc_result
zander_to_igc(FILE *in, FILE *out);

#endif
