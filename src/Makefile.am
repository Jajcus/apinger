
EXTRA_DIST = cfgparser1.h apinger.conf
EXTRA_PROGRAMS = cfgtest
sbin_PROGRAMS = apinger

apinger_SOURCES = \
		apinger.c \
		apinger.h \
		cfgparser1.y \
		cfgparser2.l \
		conf.c \
		conf.h \
		debug.h \
		icmp.c \
		icmp6.c \
		main.c

cfgtest_SOURCES = \
		cfgparser1.y \
		cfgparser2.l \
		cfgtest.c \
		conf.c 

YFLAGS=-d
