
CFLAGS=-g -Wall 
LIBS=
LDFLAGS=

SOURCES=main.c apinger.c config.c icmp.c icmp6.c
OBJS=$(SOURCES:.c=.o)

apinger: $(OBJS)
	gcc -o $@ $(LDFLAGS) $(LIBS) $(OBJS)

tags:
	ctags *.c

clean:
	rm *.o apinger
	
$(OBJS): apinger.h

.c.o:	
	gcc -o $@ $(CFLAGS) -c $<
