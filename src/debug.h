#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <time.h>

extern int foreground;
#define log(format,args...) \
		do { \
			if (foreground){ \
				time_t _t_=time(NULL);\
				static char _buf_[100];\
				strftime(_buf_,100,"%b %d %H:%M:%S",localtime(&_t_)); \
				fprintf(stderr,"[%s] ",_buf_); \
				fprintf(stderr,format,## args); \
				fprintf(stderr,"\n"); \
			}\
			else \
				syslog(LOG_ERR,format,## args); \
		}while (0)

#ifdef conf_h
# define debug(format,args...) \
		do { \
			if (config->debug) { \
				if (foreground){ \
					time_t _t_=time(NULL);\
					static char _buf_[100];\
					strftime(_buf_,100,"%b %d %H:%M:%S",localtime(&_t_)); \
					fprintf(stderr,"[%s] ",_buf_); \
					fprintf(stderr,format,## args); \
					fprintf(stderr,"\n"); \
				} \
				else \
					syslog(LOG_DEBUG,format,## args); \
			} \
		}while (0)
#endif

#define myperror(prefix) \
		do { \
			if (foreground) \
				perror(prefix); \
			else \
				syslog(LOG_ERR,"%s: %s",prefix,strerror(errno)); \
		}while(0)

