CC := $(firstword $(shell which gcc-4.0 gcc-3.4 gcc-3.3 gcc cc))
CXX := $(firstword $(shell which g++-4.0 g++-3.4 g++-3.3 g++ c++ cc))

COMMON_CFLAGS = -O0 -g
COMMON_CFLAGS += -Wall -W -pedantic
COMMON_CFLAGS += -Werror -pedantic-errors

CFLAGS = $(COMMON_CFLAGS)
CFLAGS += -std=gnu99
CFLAGS += -Wmissing-prototypes -Wwrite-strings -Wcast-qual -Wfloat-equal -Wshadow -Wpointer-arith -Wbad-function-cast -Wsign-compare -Waggregate-return -Wmissing-declarations -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Wnested-externs -Winline -Wdisabled-optimization -Wno-long-long -Wstrict-prototypes -Wundef

CXXFLAGS += $(COMMON_CFLAGS)
CXXFLAGS += -Wwrite-strings -Wcast-qual -Wfloat-equal -Wshadow -Wpointer-arith -Wsign-compare -Wmissing-noreturn -Wmissing-format-attribute -Wredundant-decls -Winline -Wdisabled-optimization -Wno-long-long -Wundef

all: src/loggerconv src/cenfistool src/hexfile src/filsertool src/fakefilser src/fwd

clean:
	rm -f src/loggerconv src/cenfistool src/hexfile src/filsertool src/fakefilser src/*.o

loggerconv_SOURCES = $(addprefix src/,conv.cc tp.cc seeyou.cc cenfis.cc cenfisdb.cc filsertp.cc zander-tp-reader.cc zander-tp-writer.cc)
loggerconv_OBJECTS = $(patsubst %.cc,%.o,$(loggerconv_SOURCES))

$(loggerconv_OBJECTS): %.o: %.cc
	$(CXX) -c $(CXXFLAGS) -o $@ $^

src/loggerconv: $(loggerconv_OBJECTS)
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
