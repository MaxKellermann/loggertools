all: loggerconv cenfistool hexfile filser

clean:
	rm -f loggerconv cenfistool hexfile filser *.o

loggerconv: conv.cc tp.cc seeyou.cc cenfis.cc cenfisdb.cc filsertp.cc
	c++ -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^ -lstdc++

cenfistool: cenfistool.c
	cc -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^

hexfile: hexfile.c
	cc -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^

filser: filser.c
	cc -Wall -W -Werror -pedantic-errors -O0 -g -o $@ $^
