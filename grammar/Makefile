all: uzbl

uzbl: uzbl.tab.o lex.yy.o
	gcc -o uzbl lex.yy.o uzbl.tab.o $(shell pkg-config glib-2.0 --libs)

lex.yy.o: lex.yy.c uzbl.tab.h
	gcc -c lex.yy.c $(shell pkg-config glib-2.0 --cflags)

uzbl.tab.o: uzbl.tab.c
	gcc -DYYDEBUG -c uzbl.tab.c $(shell pkg-config glib-2.0 --cflags)

uzbl.tab.c uzbl.tab.h: uzbl.y
	bison --defines uzbl.y

lex.yy.c: uzbl.l
	flex uzbl.l
