#
#  loggertools
#  Copyright (C) 2004-2006 Max Kellermann <max@duempel.org>
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; version 2 of the License
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.
#
#  $Id$
#

CC := $(firstword $(shell which gcc-4.1 gcc-4.0 gcc-3.4 gcc-3.3 gcc cc))
CXX := $(firstword $(shell which g++-4.1 g++-4.0 g++-3.4 g++-3.3 g++ c++ cc))

COMMON_CFLAGS = -O0 -g
COMMON_CFLAGS += -Wall -W -pedantic
COMMON_CFLAGS += -Werror -pedantic-errors

CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -std=gnu99
CFLAGS += -Wmissing-prototypes -Wwrite-strings -Wcast-qual -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast -Wsign-compare -Waggregate-return -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Wnested-externs -Winline -Wdisabled-optimization -Wno-long-long -Wstrict-prototypes -Wundef

CXXFLAGS += $(COMMON_CFLAGS)
CXXFLAGS += -Wwrite-strings -Wcast-qual -Wfloat-equal -Wshadow -Wpointer-arith -Wsign-compare -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Winline -Wdisabled-optimization -Wno-long-long -Wundef

all: src/loggerconv src/asconv src/cenfistool src/hexfile src/filsertool src/fakefilser src/fwd

clean:
	rm -f src/loggerconv src/asconv src/cenfistool src/hexfile src/filsertool src/fakefilser src/*.o

loggerconv_SOURCES = $(addprefix src/,conv.cc earth.cc tp.cc seeyou.cc cenfis.cc cenfisdb.cc filsertp.cc zander-tp-reader.cc zander-tp-writer.cc)
loggerconv_OBJECTS = $(patsubst %.cc,%.o,$(loggerconv_SOURCES))

asconv_SOURCES = $(addprefix src/,asconv.cc airspace.cc)
asconv_OBJECTS = $(patsubst %.cc,%.o,$(asconv_SOURCES))

$(loggerconv_OBJECTS): %.o: %.cc
	$(CXX) -c $(CXXFLAGS) -o $@ $^

src/loggerconv: $(loggerconv_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++

src/asconv: $(asconv_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++

src/cenfistool: src/cenfistool.c src/cenfis.c src/serialio.c
	$(CC) $(CFLAGS) -o $@ $^

src/hexfile: src/hexfile.c
	$(CC) $(CFLAGS) -o $@ $^

src/filsertool: src/filsertool.c src/filser-crc.c src/filser-open.c src/filser-io.c src/filser-proto.c src/datadir.c
	$(CC) $(CFLAGS) -o $@ $^

src/fakefilser: src/fakefilser.c src/filser-crc.c src/filser-open.c src/filser-io.c src/datadir.c
	$(CC) $(CFLAGS) -o $@ $^

src/fwd: src/fwd.c src/filser-proto.c
	$(CC) $(CFLAGS) -o $@ $^
