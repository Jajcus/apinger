# $Id: Makefile.am,v 1.9 2002/07/17 17:53:55 cvs-jajcus Exp $

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
		main.c \
		debug.c \
		tv_macros.h


cfgtest_SOURCES = \
		cfgparser1.y \
		cfgparser2.l \
		cfgtest.c \
		conf.c 

AM_CFLAGS=-D"SYSCONFDIR=\"$(sysconfdir)\"" -D"LOCALSTATEDIR=\"$(localstatedir)\""

YFLAGS=-d
