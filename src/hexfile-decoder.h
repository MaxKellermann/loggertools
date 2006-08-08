/*
 * loggertools
 * Copyright (C) 2004-2006 Max Kellermann <max@duempel.org>
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

#ifndef __HEXFILE_DECODER_H
#define __HEXFILE_DECODER_H

#include <sys/types.h>

typedef int (*hexfile_decoder_callback_t)(void *ctx,
                                          unsigned char type, unsigned offset,
                                          unsigned char *data, size_t length);

struct hexfile_decoder;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int hexfile_decoder_new(hexfile_decoder_callback_t callback, void *ctx,
                        struct hexfile_decoder **hfd_r);

int hexfile_decoder_close(struct hexfile_decoder **hfd_r);

int hexfile_decoder_feed(struct hexfile_decoder *hfd,
                         char *p, size_t length);

#ifdef __cplusplus
}
#endif

#endif
