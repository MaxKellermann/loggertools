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

#include "airspace-io.hh"

#include <string.h>

static const OpenAirAirspaceFormat openAirFormat;
static const CenfisAirspaceFormat cenfisFormat;
static const CenfisHexAirspaceFormat cenfisHexFormat;
static const CenfisTextAirspaceFormat cenfisTextFormat;
static const ZanderAirspaceFormat zanderFormat;
static const SVGAirspaceFormat svgFormat;

const AirspaceFormat *getAirspaceFormat(const char *ext) {
    if (strcasecmp(ext, "txt") == 0 || strcmp(ext, "openair") == 0)
        return &openAirFormat;
    else if (strcasecmp(ext, "asc") == 0 || strcmp(ext, "cenfis") == 0)
        return &cenfisFormat;
    else if (strcasecmp(ext, "bhf") == 0)
        return &cenfisHexFormat;
    else if (strcasecmp(ext, "asa") == 0 || strcasecmp(ext, "asb") == 0)
        return &cenfisTextFormat;
    else if (strcasecmp(ext, "az") == 0)
        return &zanderFormat;
    else if (strcasecmp(ext, "svg") == 0)
        return &svgFormat;
    else
        return NULL;
}
