user-sh:bison.tab.o execute.o lex.yy.o global.h
	gcc -o user-sh bison.tab.o execute.o lex.yy.o -lfl
bison.tab.c: bison.y
	bison -d bison.y
bison.tab.o:bison.tab.c
	gcc -c bison.tab.c
execute.o:execute.c
	gcc -c execute.c
lex.yy.o:lex.yy.c
	gcc -c lex.yy.c -lfl
lex.yy.c:flex.l
	flex flex.l
clean:
	rm user-sh *.o bison.tab.c lex.yy.c
