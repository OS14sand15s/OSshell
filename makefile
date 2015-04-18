user-sh:
	bison -d bison.y
	flex flex.l
	gcc -c bison.tab.c
	gcc -c execute.c
	gcc -c lex.yy.c
	gcc -o user-sh bison.tab.o execute.o lex.yy.o -lfl

clean:
	rm user-sh *.o bison.tab.c
