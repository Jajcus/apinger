# $Id: configure.ac,v 1.17 2002/09/25 10:25:49 cvs-jajcus Exp $

# Process this file with autoconf to produce a configure script.
AC_INIT([apinger], [0.4-test3], [jajcus@pld.org.pl])
AC_CONFIG_SRCDIR([src/apinger.c])
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AM_PROG_LEX

# Checks for libraries.
AC_ARG_WITH(efence,AC_HELP_STRING([--with-efence],[use efence (default is NO)]),[
 	if test "x$with_efence" != "xno" ; then
 		AC_CHECK_LIB([efence], [malloc])
 	fi
])

# Checks for header files.
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h errno.h malloc.h \
		stddef.h stdlib.h string.h sys/socket.h \
		sys/time.h syslog.h unistd.h time.h \
		assert.h sys/poll.h signal.h pwd.h grp.h stdarg.h])
AC_HEADER_TIME

JK_LINUX_FILTER
JK_AP_INET
JK_AP_INET6

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset socket \
strdup strerror strpbrk poll vsyslog time],
	[],AC_MSG_ERROR(some needed function is missing))

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
