all: loggerconv cenfis hexfile filser

clean:
	rm -f loggerconv cenfis hexfile filser *.o

loggerconv: conv.cc tp.cc seeyou.cc cenfis.cc
	cc -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^ -lstdc++

cenfis: cenfis.c
	cc -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^

hexfile: hexfile.c
	cc -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^

filser: filser.c
	cc -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^
