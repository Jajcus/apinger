
CFLAGS=-g -Wall `glib-config --cflags`
LIBS=`glib-config --libs` -lefence
LDFLAGS=

SOURCES=pinger.c
OBJS=$(SOURCES:.c=.o)

pinger: $(OBJS)
	gcc -o $@ $(LDFLAGS) $(LIBS) $(OBJS)

tags:
	ctags *.c

.c.o:
	gcc -o $@ $(CFLAGS) -c $<
