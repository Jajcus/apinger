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
		sys/time.h syslog.h unistd.h netinet/ip.h \
		netinet/ip_icmp.h \
		assert.h sys/poll.h signal.h pwd.h grp.h stdarg.h])

AC_CHECK_HEADERS([linux/types.h],HAS_LINUX_TYPES_H=yes)
if test "x$HAS_LINUX_TYPES_H" = "xyes" ; then
	AC_CHECK_HEADERS([linux/filter.h],[],[],[
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <linux/types.h>
])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_TYPES([struct sockaddr_in],,AC_MSG_ERROR(some needed type is missing),[
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
])

AC_CHECK_TYPES([struct sockaddr_in],,AC_MSG_ERROR(some needed type is missing),[
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
])

AC_CHECK_MEMBERS([struct icmphdr.type,struct icmphdr.code,struct icmphdr.checksum,struct icmphdr.un.echo.sequence,struct icmphdr.un.echo.id],,
		  AC_MSG_ERROR(struct icmphdr not defined or not compatible)	  
		  ,[
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
])

AC_MSG_NOTICE([checking for IPv6 support])
AC_CHECK_HEADERS([netinet/icmp6.h netinet/ip6.h])

AC_CHECK_TYPES([struct sockaddr_in6],,[HAVE_IPV6=no],[
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
])

if test "x$HAVE_IPV6" != "xno"; then
AC_CHECK_MEMBERS([struct icmp6_hdr.icmp6_type,struct icmp6_hdr.icmp6_code,
struct icmp6_hdr.icmp6_cksum, struct icmp6_hdr.icmp6_seq,
struct icmp6_hdr.icmp6_id],,[HAVE_IPV6=no],[
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>
])
fi

if test "x$HAVE_IPV6" = "xno"; then
	AC_MSG_WARN([No IPv6 support in apinger for this system])
else
	AC_DEFINE(HAVE_IPV6,1,[Define for IPv6 support])
fi

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_TYPE_SIGNAL
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([gettimeofday inet_ntoa memset socket strdup strerror strpbrk poll vsyslog],[],
		AC_MSG_ERROR(some needed function is missing))

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
