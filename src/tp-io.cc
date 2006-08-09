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

#include "tp.hh"
#include "tp-io.hh"

TurnPointFilter::~TurnPointFilter(void) {
}


static const SeeYouTurnPointFormat seeYouFormat;
static const CenfisTurnPointFormat cenfisFormat;
static const CenfisDatabaseFormat cenfisDatabaseFormat;
static const CenfisHexTurnPointFormat cenfisHexFormat;
static const FilserTurnPointFormat filserFormat;
static const ZanderTurnPointFormat zanderFormat;

const TurnPointFormat *getTurnPointFormat(const char *ext) {
    if (strcasecmp(ext, "cup") == 0)
        return &seeYouFormat;
    else if (strcasecmp(ext, "cdb") == 0 ||
             strcasecmp(ext, "idb") == 0)
        return &cenfisFormat;
    else if (strcasecmp(ext, "dab") == 0)
        return &cenfisDatabaseFormat;
    else if (strcasecmp(ext, "bhf") == 0)
        return &cenfisHexFormat;
    else if (strcasecmp(ext, "da4") == 0)
        return &filserFormat;
    else if (strcasecmp(ext, "wz") == 0)
        return &zanderFormat;
    else
        return NULL;
}

static const DistanceTurnPointFilter distanceFilter;

const TurnPointFilter *getTurnPointFilter(const char *name) {
    if (strcmp(name, "distance") == 0)
        return &distanceFilter;
    else
        return NULL;
}
