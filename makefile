user-sh: bison.tab.o execute.o
	cc -o $@ $^
bison.tab.o: bison.tab.c global.h
execute.o: execute.c global.h
bison.tab.c: bison.y
	bison $^
clean:
	rm user-sh *.o bison.tab.c
