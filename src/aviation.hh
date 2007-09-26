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

#ifndef __LOGGERTOOLS_AVIATION_HH
#define __LOGGERTOOLS_AVIATION_HH

/** a UHF radio frequency */
class Frequency {
private:
    unsigned hertz;
public:
    Frequency():hertz(0) {}
    Frequency(unsigned _hertz):hertz(_hertz) {}
    Frequency(unsigned mhz, unsigned khz):hertz((mhz * 1000 + khz) * 1000) {}
public:
    bool defined() const {
        return hertz > 0;
    }
    unsigned getHertz() const {
        return hertz;
    }
    unsigned getMegaHertz() const {
        return hertz / 1000000;
    }
    unsigned getKiloHertz() const {
        return hertz / 1000;
    }
    unsigned getKiloHertzPart() const {
        return getKiloHertz() % 1000;
    }
};

#endif
