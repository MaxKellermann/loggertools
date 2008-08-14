#
#  loggertools
#  Copyright (C) 2004-2007 Max Kellermann <max@duempel.org>
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

CC := gcc
CXX := g++
LD := ld
FAKEROOT = fakeroot

PREFIX = /usr/local

VERSION = $(shell ./bin/version)

COMMON_CFLAGS = -O0 -g
COMMON_CFLAGS += -Wall -W -pedantic
COMMON_CFLAGS += -Werror -pedantic-errors
COMMON_CFLAGS += -funit-at-a-time

CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -std=gnu99
CFLAGS += -Wmissing-prototypes -Wcast-qual -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast -Wsign-compare -Waggregate-return -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Wnested-externs -Winline -Wdisabled-optimization -Wno-long-long -Wstrict-prototypes -Wundef

CXXFLAGS += $(COMMON_CFLAGS)
CXXFLAGS += -Wwrite-strings -Wcast-qual -Wfloat-equal -Wpointer-arith -Wsign-compare -Wmissing-format-attribute -Wredundant-decls -Winline -Wdisabled-optimization -Wno-long-long -Wundef

bin_PROGRAMS = bin/tpconv \
	bin/cenfis-upload bin/hexfile \
	bin/filsertool bin/lxn-logger bin/lxn2igc \
	bin/zander bin/zander-logger bin/zan2igc bin/igc2zan
MANPAGES = \
	doc/cenfis-upload.1 \
	doc/filsertool.1 doc/lxn-logger.1 doc/lxn2igc.1 doc/lo4-logger.1 \
	doc/zander-logger.1

all: bin/tpconv bin/asconv bin/cenfis-upload bin/hexfile bin/lxn2igc bin/filsertool bin/lxn-logger bin/lo4-logger bin/fakefilser bin/flarmtool bin/zander bin/zander-logger bin/zan2igc bin/igc2zan bin/fakezander bin/lxn-fwd bin/fwd

clean:
	rm -rf bin
	rm -f $(addprefix doc/loggertools.,aux dvi log out toc)

bin/stamp:
	mkdir -p bin
	touch bin/stamp

C_HEADERS := $(wildcard src/*.h)
CC_HEADERS := $(wildcard src/*.hh)

tpconv_SOURCES = $(addprefix src/,tp-conv.cc \
	earth.cc earth-parser.cc \
	tp.cc tp-io.cc \
	tp-fancy.cc \
	tp-milomei.cc \
	tp-cenfis-reader.cc tp-cenfis-writer.cc \
	tp-cenfis-db-reader.cc tp-cenfis-db-writer.cc \
	tp-cenfis-hex-reader.cc tp-cenfis-hex-writer.cc \
	tp-seeyou-reader.cc tp-seeyou-writer.cc \
	tp-filser-reader.cc tp-filser-writer.cc \
	tp-zander-reader.cc tp-zander-writer.cc \
	tp-name.cc \
	tp-distance.cc \
	tp-airfield.cc \
	hexfile-writer.cc)
tpconv_OBJECTS = $(patsubst src/%.cc,bin/%.o,$(tpconv_SOURCES))

asconv_SOURCES = $(addprefix src/,airspace-conv.cc \
	earth.cc \
	airspace.cc airspace-io.cc \
	airspace-openair-reader.cc airspace-openair-writer.cc \
	airspace-cenfis-writer.cc \
	airspace-cenfis-hex-writer.cc \
	airspace-cenfis-txt-reader.cc \
	airspace-zander-writer.cc \
	airspace-svg-writer.cc \
	hexfile-writer.cc \
	cenfis-buffer.cc \
	cenfis-crypto.c \
	cenfis-key.c \
	)
asconv_OBJECTS = $(patsubst src/%.cc,bin/%.o,$(asconv_SOURCES))

cenfistool_SOURCES = src/cenfis-tool.c src/cenfis.c src/serialio.c
cenfistool_OBJECTS = $(patsubst src/%.c,bin/%.o,$(cenfistool_SOURCES))

cenfis_upload_SOURCES = src/cenfis-upload.c src/cenfis.c src/serialio.c
cenfis_upload_OBJECTS = $(patsubst src/%.c,bin/%.o,$(cenfis_upload_SOURCES))

lxn2igc_SOURCES = src/lxn2igc.c src/lxn-reader.c src/lxn-to-igc.c
lxn2igc_OBJECTS = $(patsubst src/%.c,bin/%.o,$(lxn2igc_SOURCES))

filsertool_SOURCES = src/filser-tool.c src/filser-crc.c src/filser-open.c src/filser-io.c src/filser-proto.c src/datadir.c src/lxn-reader.c src/lxn-to-igc.c
filsertool_OBJECTS = $(patsubst src/%.c,bin/%.o,$(filsertool_SOURCES))

lxn_logger_SOURCES = src/lxn-logger.c src/filser-crc.c src/filser-open.c src/filser-io.c src/filser-proto.c src/filser-filename.c src/lxn-reader.c src/lxn-to-igc.c
lxn_logger_OBJECTS = $(patsubst src/%.c,bin/%.o,$(lxn_logger_SOURCES))

lo4_logger_SOURCES = src/lo4-logger.c src/filser-crc.c src/filser-open.c src/filser-io.c src/filser-proto.c
lo4_logger_OBJECTS = $(patsubst src/%.c,bin/%.o,$(lo4_logger_SOURCES))

fakefilser_SOURCES = src/fakefilser.c src/filser-crc.c src/filser-open.c src/filser-io.c src/datadir.c src/lxn-reader.c src/dump.c
fakefilser_OBJECTS = $(patsubst src/%.c,bin/%.o,$(fakefilser_SOURCES))

flarmtool_SOURCES = src/flarm-tool.c src/flarm-crc.c src/flarm-open.c src/flarm-escape.c src/fifo-buffer.c src/flarm-buffer.c src/flarm-send.c src/flarm-message.c src/flarm-recv.c src/flarm-mode.c src/flarm-error.c
flarmtool_OBJECTS = $(patsubst src/%.c,bin/%.o,$(flarmtool_SOURCES))

zander_SOURCES = src/zander-tool.c src/zander-open.c src/zander-io.c src/zander-error.c src/zander-protocol.c
zander_OBJECTS = $(patsubst src/%.c,bin/%.o,$(zander_SOURCES))

zander_logger_SOURCES = src/zander-logger.c src/zander-open.c src/zander-io.c src/zander-error.c src/zander-protocol.c
zander_logger_OBJECTS = $(patsubst src/%.c,bin/%.o,$(zander_logger_SOURCES))

zan2igc_SOURCES = src/zan2igc.c src/zander-igc.c
zan2igc_OBJECTS = $(patsubst src/%.c,bin/%.o,$(zan2igc_SOURCES))

igc2zan_SOURCES = src/igc2zan.c
igc2zan_OBJECTS = $(patsubst src/%.c,bin/%.o,$(igc2zan_SOURCES))

fakezander_SOURCES = src/fakezander.c src/zander-open.c src/datadir.c src/dump.c
fakezander_OBJECTS = $(patsubst src/%.c,bin/%.o,$(fakezander_SOURCES))

version_SOURCES = src/version.c
version_OBJECTS = $(patsubst src/%.c,bin/%.o,$(version_SOURCES))

c_SOURCES = $(wildcard src/*.c)
c_OBJECTS = $(patsubst src/%.c,bin/%.o,$(c_SOURCES))

cxx_SOURCES = $(wildcard src/*.cc)
cxx_OBJECTS = $(patsubst src/%.cc,bin/%.o,$(cxx_SOURCES))

$(c_OBJECTS): bin/%.o: src/%.c bin/stamp $(C_HEADERS)
	$(CC) -c $(CFLAGS) -o $@ $<

$(cxx_OBJECTS): bin/%.o: src/%.cc bin/stamp $(CC_HEADERS)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

bin/tpconv: $(tpconv_OBJECTS) bin/hexfile-decoder.o
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++

bin/asconv: $(asconv_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++

bin/cenfistool: $(cenfistool_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/cenfis-upload: $(cenfis_upload_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/hexfile: bin/hexfile-tool.o bin/hexfile-decoder.o
	$(CC) $(CFLAGS) -o $@ $^

bin/lxn2igc: $(lxn2igc_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/filsertool: $(filsertool_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/lxn-logger: $(lxn_logger_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/lo4-logger: $(lo4_logger_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/fakefilser: $(fakefilser_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/flarmtool: $(flarmtool_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/zander: $(zander_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/zander-logger: $(zander_logger_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/zan2igc: $(zan2igc_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/igc2zan: $(igc2zan_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/fakezander: $(fakezander_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

bin/lxn-fwd: src/lxn-fwd.c bin/filser-open.o bin/filser-proto.o
	$(CC) $(CFLAGS) -o $@ $^

bin/fwd: src/fwd.c
	$(CC) $(CFLAGS) -o $@ $^

bin/version: $(version_OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

#
# documentation
#

.PHONY: documentation

documentation: doc/loggertools.pdf

doc/loggertools.pdf: doc/loggertools.tex
	cd doc && pdflatex loggertools.tex && pdflatex loggertools.tex

#
# installation
#

.PHONY: install
install: $(bin_PROGRAMS) documentation
	install -d -m 0755 $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(PREFIX)/share/man/man1 $(DESTDIR)$(PREFIX)/share/doc/loggertools
	install -m 0755 $(bin_PROGRAMS) $(DESTDIR)$(PREFIX)/bin
	install -m 0644 $(MANPAGES) $(DESTDIR)$(PREFIX)/share/man/man1
	install -m 0644 doc/loggertools.pdf $(DESTDIR)$(PREFIX)/share/doc/loggertools

#
# packages
#

.PHONY: svn-export dist

dist: documentation svn-export
	cp doc/loggertools.pdf /tmp/loggertools-$(VERSION)/doc/
	cd /tmp && $(FAKEROOT) tar cjf loggertools-$(VERSION).tar.bz2 loggertools-$(VERSION)

svn-export: bin/version
	rm -rf /tmp/loggertools-$(VERSION)
	svn export . /tmp/loggertools-$(VERSION)
