# Process this file with autoconf to produce a configure script.
AC_INIT([src/apinger.c])
AM_INIT_AUTOMAKE(apinger, 0.1, jajcus@pld.org.pl)
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
AC_CHECK_HEADERS([arpa/inet.h errno.h malloc.h netinet/in.h \
		stddef.h stdlib.h string.h sys/socket.h \
		sys/time.h syslog.h unistd.h],[],
		AC_MSG_ERROR("some needed header file is missing")
		)
AC_CHECK_HEADERS([netinet/icmp6.h netinet/ip6.h assert.h])
AC_CHECK_HEADERS([linux/types.h],HAS_TYPES_H=yes)
if test "x$HAS_TYPES_H" = "xyes" ; then
	AC_CHECK_HEADERS([linux/filter.h],[],[],
#include <linux/types.h>
)
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_TYPES([struct sockaddr_in],,AC_MSG_ERROR("some needed type is missing"),[
#include <sys/socket.h>
#include <netinet/in.h>
])
AC_CHECK_TYPES([struct sockaddr_in6],,,[
#include <sys/socket.h>
#include <netinet/in.h>
])

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset socket strdup strerror strpbrk],[],
		AC_MSG_ERROR("some needed function is missing")
		)

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
