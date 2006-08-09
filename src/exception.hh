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

#ifndef __LOGGERTOOLS_EXCEPTION_HH
#define __LOGGERTOOLS_EXCEPTION_HH

#include <stdexcept>

class already_flushed : public std::logic_error {
public:
    already_flushed():std::logic_error("Writer was already flushed") {}
    virtual ~already_flushed() throw() {}
};

class malformed_input : public std::exception {
private:
    std::string msg;
public:
    malformed_input():msg("malformed input file") {}
    malformed_input(const std::string &_msg):msg(_msg) {}
    virtual ~malformed_input() throw() {}
public:
    virtual const char *what() const throw() {
        return msg.c_str();
    }
};

class container_full : public std::exception {
private:
    std::string msg;
public:
    container_full():msg("container cannot hold more") {}
    container_full(const std::string &_msg):msg(_msg) {}
    virtual ~container_full() throw() {}
public:
    virtual const char *what() const throw() {
        return msg.c_str();
    }
};

#endif
