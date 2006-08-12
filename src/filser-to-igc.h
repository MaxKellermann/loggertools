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

#ifndef __FILSER_TO_IGC_H
#define __FILSER_TO_IGC_H

#include <stdio.h>

struct filser_to_igc;

int filser_to_igc_open(FILE *igc, struct filser_to_igc **fti_r);

int filser_to_igc_close(struct filser_to_igc **fti_r);

int filser_to_igc_process(struct filser_to_igc *fti,
                          const unsigned char *fil,
                          size_t fil_length, size_t *fil_consumed_r);

#endif
