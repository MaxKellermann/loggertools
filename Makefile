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
LD := ld

COMMON_CFLAGS = -O0 -g
COMMON_CFLAGS += -Wall -W -pedantic
COMMON_CFLAGS += -Werror -pedantic-errors

CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -std=gnu99
CFLAGS += -Wmissing-prototypes -Wcast-qual -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast -Wsign-compare -Waggregate-return -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Wnested-externs -Winline -Wdisabled-optimization -Wno-long-long -Wstrict-prototypes -Wundef

CXXFLAGS += $(COMMON_CFLAGS)
CXXFLAGS += -Wwrite-strings -Wcast-qual -Wfloat-equal -Wpointer-arith -Wsign-compare -Wmissing-format-attribute -Wredundant-decls -Winline -Wdisabled-optimization -Wno-long-long -Wundef

all: bin/tpconv bin/asconv bin/cenfistool bin/hexfile bin/filsertool bin/lxui bin/lxigc bin/fakefilser bin/fwd

clean:
	rm -rf bin

bin/stamp:
	mkdir -p bin
	touch bin/stamp

tpconv_SOURCES = $(addprefix src/,tp-conv.cc earth.cc tp.cc tp-io.cc tp-cenfis-reader.cc tp-cenfis-writer.cc tp-cenfis-db-reader.cc tp-cenfis-db-writer.cc tp-cenfis-hex-reader.cc tp-cenfis-hex-writer.cc tp-seeyou-reader.cc tp-seeyou-writer.cc tp-filser-reader.cc tp-filser-writer.cc tp-zander-reader.cc tp-zander-writer.cc tp-distance.cc)
tpconv_OBJECTS = $(patsubst src/%.cc,bin/%.o,$(tpconv_SOURCES))

asconv_SOURCES = $(addprefix src/,airspace-conv.cc earth.cc airspace.cc airspace-io.cc airspace-openair-reader.cc airspace-openair-writer.cc)
asconv_OBJECTS = $(patsubst src/%.cc,bin/%.o,$(asconv_SOURCES))

cenfistool_SOURCES = src/cenfis-tool.c src/cenfis.c src/serialio.c
cenfistool_OBJECTS = $(patsubst src/%.c,bin/%.o,$(cenfistool_SOURCES))

filsertool_SOURCES = src/filser-tool.c src/filser-crc.c src/filser-open.c src/filser-io.c src/filser-proto.c src/datadir.c src/lxn-reader.c src/lxn-to-igc.c
filsertool_OBJECTS = $(patsubst src/%.c,bin/%.o,$(filsertool_SOURCES))

lxui_SOURCES = src/lx-ui.c src/lx-ui-device.c src/lx-ui-setup.c src/lx-ui-flight-info.c src/lx-ui-igc.c src/filser-crc.c src/filser-open.c src/filser-io.c src/filser-proto.c src/lxn-reader.c src/lxn-to-igc.c
lxui_OBJECTS = $(patsubst src/%.c,bin/%.o,$(lxui_SOURCES))

lxigc_SOURCES = src/lxigc.c src/filser-crc.c src/filser-open.c src/filser-io.c src/filser-proto.c src/filser-filename.c src/lxn-reader.c src/lxn-to-igc.c
lxigc_OBJECTS = $(patsubst src/%.c,bin/%.o,$(lxigc_SOURCES))

fakefilser_SOURCES = src/fakefilser.c src/filser-crc.c src/filser-open.c src/filser-io.c src/datadir.c src/lxn-reader.c
fakefilser_OBJECTS = $(patsubst src/%.c,bin/%.o,$(fakefilser_SOURCES))

c_SOURCES = $(wildcard src/*.c)
c_OBJECTS = $(patsubst src/%.c,bin/%.o,$(c_SOURCES))

cxx_SOURCES = $(wildcard src/*.cc)
cxx_OBJECTS = $(patsubst src/%.cc,bin/%.o,$(cxx_SOURCES))

$(c_OBJECTS): bin/%.o: src/%.c bin/stamp
	$(CC) -c $(CFLAGS) -o $@ $<

$(cxx_OBJECTS): bin/%.o: src/%.cc bin/stamp
	$(CXX) -c $(CXXFLAGS) -o $@ $<

bin/libhexfile.a: bin/hexfile-decoder.o
	$(LD) -r -o $@ $^

bin/tpconv: $(tpconv_OBJECTS) bin/libhexfile.a
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++

bin/asconv: $(asconv_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++

bin/cenfistool: $(cenfistool_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/hexfile: bin/hexfile-tool.o bin/libhexfile.a
	$(CC) $(CFLAGS) -o $@ $^

bin/filsertool: $(filsertool_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/lxui: $(lxui_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lnewt

bin/lxigc: $(lxigc_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/fakefilser: $(fakefilser_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/fwd: src/fwd.c bin/filser-open.o bin/filser-proto.o
	$(CC) $(CFLAGS) -o $@ $^
