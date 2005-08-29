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

all: loggerconv cenfistool hexfile filsertool fakefilser

clean:
	rm -f loggerconv cenfistool hexfile filsertool fakefilser *.o

loggerconv: conv.cc tp.cc seeyou.cc cenfis.cc cenfisdb.cc filsertp.cc
	$(CXX) $(CXXFLAGS) -o $@ $^ -lstdc++

cenfistool: cenfistool.c cenfis.c serialio.c
	$(CC) $(CFLAGS) -o $@ $^

hexfile: hexfile.c
	$(CC) $(CFLAGS) -o $@ $^

filsertool: filsertool.c filser.c filserio.c
	$(CC) $(CFLAGS) -o $@ $^

fakefilser: fakefilser.c filser.c filserio.c
	$(CC) $(CFLAGS) -o $@ $^
