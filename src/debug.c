#include "config.h"
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
#ifdef HAVE_TIME_H
# include <time.h>
#endif
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif

#include "apinger.h"

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
