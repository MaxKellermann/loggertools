all: loggerconv

clean:
	rm -f loggerconv *.o

loggerconv: conv.cc tp.cc seeyou.cc
	cc -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^ -lstdc++
