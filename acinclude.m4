AC_PREREQ([2.52])

# JK_LINUX_FILTER
# Test for Linux socket filtering
# -----------------------------------------------------
AC_DEFUN([JK_LINUX_FILTER],
[AC_CHECK_HEADERS([linux/types.h],HAS_LINUX_TYPES_H=yes)
if test "x$HAS_LINUX_TYPES_H" = "xyes" ; then
	AC_CHECK_HEADERS([linux/filter.h],[],[],[
#include <sys/types.h>
#include <unistd.h>
#include <linux/types.h>
])
fi
])

# JK_AP_INET
# Test for IPv4, ICMP services needed for apinger
# -----------------------------------------------------
AC_DEFUN([JK_AP_INET],
[ AC_CHECK_HEADERS([sys/types.h],[jk_inet_includes="$jk_inet_includes
#include <sys/types.h>"])
AC_CHECK_HEADERS([netinet/in_systm.h],[jk_inet_includes="$jk_inet_includes
#include <netinet/in_systm.h>"],,[$jk_inet_includes])
AC_CHECK_HEADERS([netinet/in.h],[jk_inet_includes="$jk_inet_includes
#include <netinet/in.h>"],,[$jk_inet_includes])
AC_CHECK_HEADERS([netinet/ip.h],[jk_icmp_includes="$jk_icmp_includes
#include <netinet/ip.h>"],,[$jk_inet_includes])
AC_CHECK_HEADERS([netinet/ip_icmp.h],[jk_icmp_includes="$jk_icmp_includes
#include <netinet/ip_icmp.h>"],,[$jk_inet_includes
$jk_icmp_includes])

AC_CHECK_TYPES([struct sockaddr_in],,AC_MSG_ERROR(some needed type is missing),[$jk_inet_includes])

AC_CHECK_MEMBERS([struct ip.ip_hl],[],
		AC_MSG_ERROR(struct ip not defined or not compatible),
		[$jk_inet_includes
$jk_icmp_includes])

AC_CHECK_MEMBERS([struct icmp.icmp_type, struct icmp.icmp_code,\
struct icmp.icmp_cksum, struct icmp.icmp_seq,\
struct icmp.icmp_id],[],
		AC_MSG_ERROR(struct icmp not defined or not compatible),
		[$jk_inet_includes
$jk_icmp_includes])
])

# JK_AP_INET6
# Test for IPv6, ICMP6 services needed for apinger
# -----------------------------------------------------
AC_DEFUN([JK_AP_INET6],
[AC_REQUIRE([JK_AP_INET])dnl
AC_CHECK_HEADERS([netinet/ip6.h],[jk_icmp6_includes="$jk_icmp6_includes
#include <netinet/ip6.h>"],,[$jk_inet_includes])
AC_CHECK_HEADERS([netinet/icmp6.h],[jk_icmp6_includes="$jk_icmp6_includes
#include <netinet/icmp6.h>"],,[$jk_inet_includes
$jk_icmp6_includes])

AC_CHECK_TYPES([struct sockaddr_in6],,[HAVE_IPV6=no],[$jk_inet_includes])

if test "x$HAVE_IPV6" != "xno"; then
AC_CHECK_MEMBERS([struct icmp6_hdr.icmp6_type,struct icmp6_hdr.icmp6_code,
struct icmp6_hdr.icmp6_cksum, struct icmp6_hdr.icmp6_seq,
struct icmp6_hdr.icmp6_id],,[HAVE_IPV6=no],[$jk_inet_includes
$jk_icmp6_includes])
fi

if test "x$HAVE_IPV6" = "xno"; then
	AC_MSG_WARN([No IPv6 support in apinger for this system])
else
	AC_DEFINE(HAVE_IPV6,1,[Define for IPv6 support])
fi

])

