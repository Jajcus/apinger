/*
 *  Alarm Pinger (c) 2002 Jacek Konieczny <jajcus@pld.org.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as published
 *  by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *  USA
 *
 *  $Id: debug.c,v 1.4 2002/07/17 09:32:51 cvs-jajcus Exp $
 */

#include "config.h"
#include "apinger.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_SYSLOG_H
# include <syslog.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif


void logit(const char *format, ...){
va_list args;

	va_start(args, format);
	if (foreground){
		time_t t=time(NULL);
		static char buf[100];
		strftime(buf,100,"%b %d %H:%M:%S",localtime(&t));
		fprintf(stderr,"[%s] ",buf);
		vfprintf(stderr, format, args);
		fprintf(stderr,"\n");
	}
	else{
		vsyslog(LOG_ERR,format,args);
	}
	va_end(args);
}

void debug(const char *format, ...){
va_list args;

	if (!config->debug) return;

	va_start(args, format);
	if (foreground){
		time_t t=time(NULL);
		static char buf[100];
		strftime(buf,100,"%b %d %H:%M:%S",localtime(&t));
		fprintf(stderr,"[%s] ",buf);
		vfprintf(stderr, format, args);
		fprintf(stderr,"\n");
	}
	else{
		vsyslog(LOG_DEBUG,format,args);
	}
	va_end(args);
}

void myperror(const char *prefix){
	if (foreground) 
		perror(prefix);
	else 
		syslog(LOG_ERR,"%s: %s",prefix,strerror(errno));
}
