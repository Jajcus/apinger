# $Id: configure.ac,v 1.20 2002/10/03 12:37:08 cvs-jajcus Exp $

# Process this file with autoconf to produce a configure script.
AC_INIT([apinger], [0.5-test1], [jajcus@pld.org.pl])
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
		assert.h sys/poll.h signal.h pwd.h grp.h stdarg.h\
		limits.h])
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
strdup strerror strpbrk poll vsyslog time popen setvbuf access],
	[],AC_MSG_ERROR(some needed function is missing))

AC_ARG_ENABLE(forked-receiver,[AC_HELP_STRING([--enable-forked-receiver],
	      			[Create subprocess for receiving pings.])],
			      		[],[enable_forked_receiver=auto])

if test "x$enable_forked_receiver" = "xyes" ; then
	AC_DEFINE(FORKED_RECEIVER,[],[Define to enable receiver subprocess])
else 
	AC_MSG_CHECKING([if PIPE_BUF is big enough])
	if test "x$enable_forked_receiver" != "xno" ; then
		AC_TRY_RUN([
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif

int main(int argc,char **argv){
	
	if (PIPE_BUF<1024) return 1;
	return 0;
}],
		[AC_DEFINE(FORKED_RECEIVER,[1],[Define to enable receiver subprocess])
		AC_MSG_RESULT([yes])],[AC_MSG_RESULT([no])
					AC_MSG_WARN([Receiver subprocess will not be used.])])
	fi
fi

AC_ARG_WITH(rrdtool,[AC_HELP_STRING([--with-rrdtool=path],[Location of rrdtool program])],
	[ RRDTOOL="$withval" ],[ AC_PATH_PROG([RRDTOOL],[rrdtool],[rrdtool]) ])
AC_ARG_WITH(rrdcgi,[AC_HELP_STRING([--with-rrdcgi=path],[Location of rrdcgi program])],
	[RRDCGI="$withval"],[AC_PATH_PROG([RRDCGI],[rrdcgi],[/usr/bin/rrdcgi])])

AC_DEFINE_UNQUOTED(RRDTOOL,\"$RRDTOOL\",[Set to the path of rrdtool program])
AC_DEFINE_UNQUOTED(RRDCGI,\"$RRDCGI\",[Set to the path of rrdcgi program])

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
