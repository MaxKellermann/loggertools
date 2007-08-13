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

#include "cenfis-crypto.h"

extern unsigned char cenfis_key_1[];

extern unsigned char cenfis_key_2[];

extern unsigned char cenfis_key_3[];

void
cenfis_encrypt(void *p0, size_t length)
{
    unsigned char *p = (unsigned char*)p0;
    size_t k = 0;

    while (length > 0) {
        *p = ((*p ^ (cenfis_key_1[k] + 60)) + cenfis_key_2[k] - 60)
            ^ (cenfis_key_3[k] + 100);
        ++k;
        if (k == 200)
            k = 0;
        ++p;
        --length;
    }
}
