
CFLAGS=-ggdb -Wall 
LIBS=
LDFLAGS=

SOURCES=main.c apinger.c config.c icmp.c icmp6.c
OBJS=$(SOURCES:.c=.o)
.SUFFIXES: .y .yy .c .o

cfgtest: cfgparser.yy.o cfgparser.tab.o conf.o
	gcc -o $@ cfgparser.yy.o cfgparser.tab.o conf.o -lfl 
	
cfgparser.yy.o: cfgparser.yy.c cfgparser.tab.h

cfgparser.yy.c: cfgparser.yy
	flex -o$@ $<

cfgparser.tab.c cfgparser.tab.h: cfgparser.y
	bison -d $<

apinger: $(OBJS)
	gcc -o $@ $(LDFLAGS) $(LIBS) $(OBJS)

tags:
	ctags *.c

clean:
	rm *.o apinger cfgtest *.output *.yy.c *.tab.c *.tab.h
	
$(OBJS): apinger.h

.c.o:	
	gcc -o $@ $(CFLAGS) -c $<
