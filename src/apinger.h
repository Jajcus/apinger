#ifndef apinger_h
#define apinger_h

#define CONFIG "/etc/apinger.conf"
#define AVG_DELAY_SAMPLES 20
#define AVG_LOSS_DELAY_SAMPLES 5
#define AVG_LOSS_SAMPLES 60
#define MAX_POLL_TIMEOUT 10000

#define PINGER_USER "pinger"
#define PID_FILE "/var/run/apinger.pid"

#include <netinet/in.h>
#include <netinet/ip6.h>

union addr {
	struct sockaddr addr;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
};

struct target_config {
	int interval;
	int alarmdown;
	int alarmdelay_high;
	int alarmdelay_low;
	int alarmloss_high;
	int alarmloss_low;
	char *mailto;
	char *mailfrom;
};
extern struct target_config current_config;

struct target {
	char *name;		/* name (IP address as string) */
	char *desc;		/* description */
	struct timeval interval_tv;

	union addr addr;	/* target address */
	char queue[AVG_LOSS_DELAY_SAMPLES+AVG_LOSS_SAMPLES]; /*
				contains info about recently sent packets
				"1" means it was received */
	int last_sent;		/* sequence number of the last ping sent */
	int last_received;	/* sequence number of the last ping received */
	int received;		/* number of packets received */
	int recently_lost;	/* number of packets lost between
				   last_sent-200 to last_sent-100
				   for avg. lost computation */
	double rbuf[AVG_DELAY_SAMPLES]; /* bufor of received pings
				   (for avarage delay computation) */
	double delay_sum;

	struct timeval next_probe; /* time when next probe is scheduled */

	int alarm_down;		/* "target is down" alarm fired */
	int alarm_loss;		/* "loss to big" alarm fired */
	int alarm_delay;	/* "delay too big" alarm fired */

	struct target *next;
	struct target_config config;
};

struct trace_info {
	struct timeval timestamp;
	int seq;
	void *target_id;
};

struct target *targets;

extern int cf_debug;
extern int foreground;

extern int icmp_sock;
extern int icmp6_sock;
extern int ident;

extern struct timeval next_probe;

#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <time.h>

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

#define debug(format,args...) \
		do { \
			if (cf_debug) { \
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

#define myperror(prefix) \
		do { \
			if (foreground) \
				perror(prefix); \
			else \
				syslog(LOG_ERR,"%s: %s",prefix,strerror(errno)); \
		}while(0)

void read_config(void);

int make_icmp_socket(void);
void recv_icmp(void);
void send_icmp_probe(struct target *t,struct timeval *cur_time);

int make_icmp6_socket(void);
void recv_icmp6(void);
void send_icmp6_probe(struct target *t,struct timeval *cur_time);

void analyze_reply(struct timeval time_recv,int seq,struct trace_info *ti);
void main_loop(void);

extern volatile int signal_received;

#endif
