#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <grp.h>
#include <malloc.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/filter.h>

#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#define CONFIG "/etc/pinger.conf"
#define AVG_DELAY_SAMPLES 20
#define AVG_LOSS_DELAY_SAMPLES 3
#define AVG_LOSS_SAMPLES 10
#define MAX_POLL_TIMEOUT 10000

#define PINGER_USER "pinger"
#define PID_FILE "/var/run/pinger.pid"

union addr {
	struct sockaddr addr;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
};

struct config {
	int interval;
	int alarmdown;
	int alarmdelay_high;
	int alarmdelay_low;
	int alarmloss_high;
	int alarmloss_low;
	char *mailto;
	char *mailfrom;
}current_config={1000,-1,-1,-1,-1,-1,NULL,NULL};

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
	struct config config;
};

struct trace_info {
	struct timeval timestamp;
	void *target_id;
};

struct target *targets=NULL;

int debug_on=0;
int foreground=0;

int icmp_sock;
int icmp6_sock;
int ident;

struct timeval next_probe={0,0};

typedef void (*sighandler_t)(int);
volatile int signal_received=0;
void signal_handler(int signum){

	signal_received=signum;
}

#define log(format,args...) \
		do { \
			if (foreground) \
				fprintf(stderr,format "\n",## args); \
			else \
				syslog(LOG_ERR,format,## args); \
		}while (0)

#define debug(format,args...) \
		do { \
			if (debug_on) { \
				if (foreground) \
					fprintf(stderr,format "\n",## args); \
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

void add_target(char *params){
union addr addr;
int r;
struct target *t;
char *name;
char *desc;

	desc=strpbrk(params," \t");
	if (desc){
		*desc++='\000';
		for(;*desc==' '||*desc=='\t';desc++);
	}
	else desc="";
	name=params;
	memset(&addr,0,sizeof(addr));
	r=inet_pton(AF_INET,name,&addr.addr4.sin_addr);
	if (r){
		if (icmp_sock<0){
			log("Sorry, IPv4 is not available\n");
			log("Ignoring target %s\n",name);
			return;
		}
		addr.addr.sa_family=AF_INET;
	}else{
		r=inet_pton(AF_INET6,name,&addr.addr6.sin6_addr);
		if (r==0){
			log("Bad host address: %s\n",name);
			exit(1);
		}
		if (icmp6_sock<0){
			log("Sorry, IPv6 is not available\n");
			log("Ignoring target %s\n",name);
			return;
		}
		addr.addr.sa_family=AF_INET6;
	}
	t=malloc(sizeof(struct target));
	assert(t!=NULL);
	memset(t,0,sizeof(struct target));
	t->name=strdup(name);
	t->desc=strdup(desc);
	t->addr=addr;
	t->next=targets;
	t->config=current_config;
	t->interval_tv.tv_usec=(current_config.interval%1000)*1000;
	t->interval_tv.tv_sec=current_config.interval/1000;
	t->last_sent=-1;
	t->last_received=-1;
	targets=t;
}

void read_config(void){
char buf[1025];
char *p;
char *param;
char *value;
char *value1;
int t;
FILE *f;

	f=fopen(CONFIG,"r");
	if (f==NULL){
		myperror("fopen: " CONFIG);
		exit(1);
	}
	while(1){
		errno=0;
		p=fgets(buf,1024,f);
		if (!p){
			if (errno==0) break;
			myperror("fgets: " CONFIG);
			exit(1);
		}
		buf[1025]=0;
		p=strpbrk(buf,"\r\n#");
		if (p) *p=0;
		for(p=buf+strlen(buf)-1;*p==' '||*p=='\t';p++) *p=0;
		for(param=buf;*param==' '||*param=='\t';param++);
		if (*param=='\000') continue;
		value=strpbrk(param," \t");
		if (value){
			*value++='\000';
			for(;*value==' '||*value=='\t';value++);
		}
		else value="";

		if (strcmp(param,"interval")==0){
			t=atoi(value);
			if (t<=0){
				log("Bad interval value '%s' in %s.\n",value,CONFIG);
				exit(1);
			}
			current_config.interval=t;
		}
		else if (strcmp(param,"alarmdown")==0){
			current_config.alarmdown=atoi(value);
			if (current_config.alarmdown<=0){
				log("Bad alarmdown value '%s' in %s.\n",value,CONFIG);
				exit(1);
			}
		}
		else if (strcmp(param,"alarmdelay")==0){
			value1=strpbrk(value,", \t");
			if (!value1){
				log("alarmdelay needs two values.\n");
				exit(1);
			}
			*value1++='\000';
			for(;*value1==' '||*value1=='\t';value1++);

			current_config.alarmdelay_low=atoi(value);
			current_config.alarmdelay_high=atoi(value1);
			if (current_config.alarmdelay_low<1
					|| current_config.alarmdelay_high<current_config.alarmdelay_low){
				log("Bad alarmdelay values '%s,%s' in %s.\n",value,value1,CONFIG);
				exit(1);
			}
		}
		else if (strcmp(param,"alarmloss")==0){
			value1=strpbrk(value,", \t");
			if (!value1){
				log("alarmloss needs two values.\n");
				exit(1);
			}
			*value1++='\000';
			for(;*value1==' '||*value1=='\t';value1++);

			current_config.alarmloss_low=atoi(value);
			current_config.alarmloss_high=atoi(value1);
			if (current_config.alarmloss_low<1
					|| current_config.alarmloss_high<current_config.alarmloss_low){
				log("Bad alarmloss values '%s,%s' in %s.\n",value,value1,CONFIG);
				exit(1);
			}
		}
		else if (strcmp(param,"mailto")==0){
			if (value[0]){
				if (current_config.mailto)
					free(current_config.mailto);
				current_config.mailto=strdup(value);
				assert(current_config.mailto!=NULL);
			}
			else {
				log("Empty mailto in %s.\n",CONFIG);
				exit(1);
			}
		}
		else if (strcmp(param,"mailfrom")==0){
			if (value[0]){
				current_config.mailfrom=strdup(value);
				assert(current_config.mailfrom!=NULL);
			}
			else {
				if (current_config.mailfrom)
					free(current_config.mailfrom);
				current_config.mailfrom=NULL;
			}
		}

		else if (strcmp(param,"target")==0){
			add_target(value);
		}
		else if (strcmp(param,"debug")==0){
			debug_on=1;
		}
		else{
			log("Unknown parameter '%s' in %s, ignoring.\n",param,CONFIG);
		}
	}
	fclose(f);
}

void toggle_alarm(struct target *t,char *name,int on){
FILE *p;
char buf[1024];
int ret;
time_t tm;
char *mailto,*mailfrom;


	if (on)
		log("ALARM: %s(%s)  *** %s ***\n",t->desc,t->name,name);
	else
		log("alarm cancelled: %s(%s)  *** %s ***\n",t->desc,t->name,name);

	mailto=t->config.mailto;
	mailfrom=t->config.mailfrom;
	if (mailto){
		snprintf(buf,1024,"/usr/lib/sendmail %s%s%s'%s'",
					mailfrom?"-f'":"",
					mailfrom?mailfrom:"",
					mailfrom?"' ":"",
					mailto);
		p=popen(buf,"w");
		if (!p){
			myperror("Couldn't send mail: popen:");
			return;
		}
		tm=time(NULL);
		if (on)
			fprintf(p,"Subject: ALARM: %s(%s) *** %s ***\n",t->desc,t->name,name);
		else
			fprintf(p,"Subject: alarm cancelled: %s(%s) *** %s ***\n",t->desc,t->name,name);
		fprintf(p,"To: %s\n",mailto);
		if (mailfrom) {
			fprintf(p,"From: Alarm Pinger <%s>\n",mailfrom);
		}
		fprintf(p,"\n%s",ctime(&tm));
		if (on)
			fprintf(p,"ALARM went off: %s\n",name);
		else
			fprintf(p,"alarm cancelled: %s\n",name);
		fprintf(p,"Target: %s\n",t->name);
		fprintf(p,"Description: %s\n",t->desc);
		fprintf(p,"Probes sent: %i\n",t->last_sent+1);
		fprintf(p,"Replies received: %i\n",t->received);
		fprintf(p,"Last reply received: %i\n",t->last_received);
		if (t->received>=AVG_DELAY_SAMPLES){
			fprintf(p,"Recent avg. delay: %4.2fms\n",t->delay_sum/AVG_DELAY_SAMPLES);
		}
		ret=pclose(p);
		if (!WIFEXITED(ret)){
			log("Error while sending mail.\n");
			log("sendmail terminated abnormally.\n");
			return;
		}
		if (WEXITSTATUS(ret)!=0){
			log("Error while sending mail.\n");
			log("sendmail exited with status: %i\n",WEXITSTATUS(ret));
			return;
		}
	}
}

/* filter instalation code borrowed from iputils */
void install_filter(){
        static struct sock_filter insns[] = {
                BPF_STMT(BPF_LDX|BPF_B|BPF_MSH, 0), /* Skip IP header. F..g BSD... Look into ping.  */
                BPF_STMT(BPF_LD|BPF_H|BPF_IND, 4), /* Load icmp echo ident */
                BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0xAAAA, 0, 1), /* Ours? */
                BPF_STMT(BPF_RET|BPF_K, ~0U), /* Yes, it passes. */
                BPF_STMT(BPF_LD|BPF_B|BPF_IND, 0), /* Load icmp type */
                BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, ICMP_ECHOREPLY, 1, 0), /* Echo? */
                BPF_STMT(BPF_RET|BPF_K, 0xFFFFFFF), /* No. It passes. */
                BPF_STMT(BPF_RET|BPF_K, 0) /* Echo with wrong ident. Reject. */
        };
        static struct sock_fprog filter = {
                sizeof insns / sizeof(insns[0]),
                insns
        };

        /* Patch bpflet for current identifier. */
        insns[2] = (struct sock_filter)BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, htons(ident), 0, 1);

        if (setsockopt(icmp_sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)))
                myperror("WARNING: failed to install socket filter\n");
}

void install_filter6(){

        static struct sock_filter insns[] = {
                BPF_STMT(BPF_LD|BPF_H|BPF_ABS, 4),  /* Load icmp echo ident */
                BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0xAAAA, 0, 1),  /* Ours? */
                BPF_STMT(BPF_RET|BPF_K, ~0U),  /* Yes, it passes. */
                BPF_STMT(BPF_LD|BPF_B|BPF_ABS, 0),  /* Load icmp type */
                BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, ICMP6_ECHO_REPLY, 1, 0), /* Echo? */
                BPF_STMT(BPF_RET|BPF_K, ~0U), /* No. It passes. This must not happen. */
                BPF_STMT(BPF_RET|BPF_K, 0), /* Echo with wrong ident. Reject. */
        };
        static struct sock_fprog filter = {
                sizeof insns / sizeof(insns[0]),
                insns
        };

        insns[1] = (struct sock_filter)BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, htons(ident), 0, 1);

        if (setsockopt(icmp6_sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)))
                myperror("WARNING: failed to install socket filter\n");
}

/* function borrowed from iputils */
u_short in_cksum(const u_short *addr, register int len, u_short csum){

	register int nleft = len;
        const u_short *w = addr;
        register u_short answer;
        register int sum = csum;

        /*
         *  Our algorithm is simple, using a 32 bit accumulator (sum),
         *  we add sequential 16 bit words to it, and at the end, fold
         *  back all the carry bits from the top 16 bits into the lower
         *  16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1)
                sum += htons(*(u_char *)w << 8);

        /*
         * add back carry outs from top 16 bits to low 16 bits
         */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return (answer);
}

void send_icmp_probe(struct target *t,struct timeval *cur_time){
static char buf[1024];
struct icmphdr *p=(struct icmphdr *)buf;
struct trace_info ti;
int size;
int ret;

	p->type=ICMP_ECHO;
	p->code=0;
	p->checksum=0;
	p->un.echo.sequence=++t->last_sent;
	p->un.echo.id=ident;

	ti.timestamp=*cur_time;
	ti.target_id=t;
	memcpy(p+1,&ti,sizeof(ti));
	size=sizeof(*p)+sizeof(ti);

	p->checksum = in_cksum((u_short *)p,size,0);
	ret=sendto(icmp_sock,p,size,MSG_DONTWAIT,
			(struct sockaddr *)&t->addr.addr4,sizeof(t->addr.addr4));
	if (ret<0){
		myperror("sendto");
	}
}

void send_icmp6_probe(struct target *t,struct timeval *cur_time){
static char buf[1024];
struct icmp6_hdr *p=(struct icmp6_hdr *)buf;
struct trace_info ti;
int size;
int ret;

	p->icmp6_type=ICMP6_ECHO_REQUEST;
	p->icmp6_code=0;
	p->icmp6_cksum=0;
	p->icmp6_seq=t->last_sent++;
	p->icmp6_id=ident;

	ti.timestamp=*cur_time;
	ti.target_id=t;
	memcpy(p+1,&ti,sizeof(ti));
	size=sizeof(*p)+sizeof(ti);

	ret=sendto(icmp6_sock,p,size,MSG_DONTWAIT,
			(struct sockaddr *)&t->addr.addr6,sizeof(t->addr.addr6));
	if (ret<0){
		myperror("sendto");
	}
}


void send_probe(struct target *t,struct timeval *cur_time){
int i,i1;
double avg_loss;

	timeradd(cur_time,&t->interval_tv,&t->next_probe);
	debug("Sending ping to %s (%s)",t->desc,t->name);
	debug("Next one scheduled for %s",ctime(&t->next_probe.tv_sec));
	if (t->addr.addr.sa_family==AF_INET) send_icmp_probe(t,cur_time);
	else if (t->addr.addr.sa_family==AF_INET6) send_icmp6_probe(t,cur_time);

	if (t->last_sent>=AVG_LOSS_DELAY_SAMPLES+AVG_LOSS_SAMPLES){
		i=t->last_sent%(AVG_LOSS_DELAY_SAMPLES+AVG_LOSS_SAMPLES);
		if (!t->queue[i]) t->recently_lost--;
		t->queue[i]=0;
	}
	if (t->last_sent>=AVG_LOSS_DELAY_SAMPLES){
		i1=(t->last_sent-AVG_LOSS_DELAY_SAMPLES)%(AVG_LOSS_DELAY_SAMPLES+AVG_LOSS_SAMPLES);
		if (!t->queue[i1]) t->recently_lost++;
		debug("Recently lost packets: %i",t->recently_lost);
	}
	if (t->last_sent>AVG_LOSS_DELAY_SAMPLES+AVG_LOSS_SAMPLES){
		avg_loss=t->recently_lost/AVG_LOSS_SAMPLES;
		if (!t->alarm_loss && avg_loss > t->config.alarmloss_low){
			t->alarm_loss=1;
			toggle_alarm(t,"packet loss too big",1);
		}
	}
}

void analyze_reply(struct timeval time_recv,int seq,struct trace_info *ti){
struct target *t;
struct timeval tv;
double delay,avg_delay,avg_loss;
double tmp;
int i;


	for(t=targets;t!=NULL;t=t->next){
		if (t==ti->target_id) break;
	}
	if (t==NULL){
		log("Couldn't match any target to the echo reply.\n");
		return;
	}
	timersub(&time_recv,&ti->timestamp,&tv);
	delay=tv.tv_sec*1000.0+((double)tv.tv_usec)/1000.0;
	debug("#%i from %s(%s) delay: %4.2fms",seq,t->desc,t->name,delay);
	if (seq>t->last_received) t->last_received=seq;
	if (t->alarm_down){
		toggle_alarm(t,"down",0);
		t->alarm_down=0;
	}
	tmp=t->rbuf[t->received%AVG_DELAY_SAMPLES];
	t->rbuf[t->received%AVG_DELAY_SAMPLES]=delay;
	t->delay_sum+=delay-tmp;
	t->received++;

	if (t->received>AVG_DELAY_SAMPLES){
		avg_delay=t->delay_sum/AVG_DELAY_SAMPLES;
		debug("(avg: %4.2fms)",avg_delay);
		if (t->config.alarmdelay_high > 0
				&& !t->alarm_delay && avg_delay>t->config.alarmdelay_high){
			t->alarm_delay=1;
			toggle_alarm(t,"delay too big",1);
		}
		else if (t->config.alarmdelay_low >0 && t->alarm_delay
				&& avg_delay<t->config.alarmdelay_low){
			t->alarm_delay=0;
			toggle_alarm(t,"delay too big",0);
		}
	}

	i=seq%(AVG_LOSS_DELAY_SAMPLES+AVG_LOSS_SAMPLES);
	if (!t->queue[i] && seq<t->last_sent-AVG_LOSS_DELAY_SAMPLES)
		t->recently_lost--;
	t->queue[i]=1;
	if (t->last_sent>AVG_LOSS_DELAY_SAMPLES+AVG_LOSS_SAMPLES){
		avg_loss=((double)t->recently_lost)/AVG_LOSS_SAMPLES;
		debug("(avg. loss: %5.1f%%)",avg_loss*100);
		if (t->alarm_loss && avg_loss < t->config.alarmloss_low){
			t->alarm_loss=0;
			toggle_alarm(t,"packet loss too big",0);
		}
	}
}

void recv_icmp(){
int len,hlen,icmplen,datalen;
char buf[1024];
struct sockaddr_in from;
struct icmphdr *icmp;
struct iphdr *ip;
struct timeval time_recv;
socklen_t sl;

	sl=sizeof(from);
	gettimeofday(&time_recv,NULL);
	len=recvfrom(icmp_sock,buf,1024,MSG_DONTWAIT,(struct sockaddr *)&from,&sl);
	if (len<0){
		if (errno==EAGAIN) return;
		myperror("recvfrom");
		return;
	}
	if (len==0) return;
	ip=(struct iphdr *)buf;
	hlen=ip->ihl*4;
	if (len<hlen+8 || ip->ihl<5) {
		debug("Too short packet reveiced");
		return;
	}
	icmplen=len-hlen;
	icmp=(struct icmphdr *)(buf+hlen);
	if (icmp->type != ICMP_ECHOREPLY) return;
	if (icmp->un.echo.id != ident) return;
	debug("Ping reply from %s",inet_ntoa(from.sin_addr));
	datalen=icmplen-sizeof(*icmp);
	if (datalen!=sizeof(struct trace_info)){
		debug("Packet data truncated.");
		return;
	}
	analyze_reply(time_recv,icmp->un.echo.sequence,(struct trace_info*)(icmp+1));
}

void recv_icmp6(){
int len,icmplen,datalen;
char buf[1024];
char abuf[100];
const char *name;
struct sockaddr_in6 from;
struct icmp6_hdr *icmp;
socklen_t sl;
struct timeval time_recv;

	sl=sizeof(from);
	gettimeofday(&time_recv,NULL);
	len=recvfrom(icmp6_sock,buf,1024,0,(struct sockaddr *)&from,&sl);
	if (len<0){
		if (errno==EAGAIN) return;
		myperror("recvfrom");
		return;
	}
	if (len==0) return;

	icmplen=len;
	icmp=(struct icmp6_hdr *)buf;
	if (icmp->icmp6_type != ICMP6_ECHO_REPLY) return;
	if (icmp->icmp6_id != ident) return;

	name=inet_ntop(AF_INET6,&from.sin6_addr,abuf,100);
	debug("Ping reply from %s",name);
	datalen=icmplen-sizeof(*icmp);
	if (datalen!=sizeof(struct trace_info)){
		debug("Packet data truncated.");
		return;
	}
	analyze_reply(time_recv,icmp->icmp6_seq,(struct trace_info*)(icmp+1));
}

void main_loop(void){
struct target *t;
struct timeval cur_time,tv;
struct pollfd pfd[2];
int timeout;
int npfd=0;
int i;

	if (icmp_sock){
		pfd[npfd].events=POLLIN|POLLERR|POLLHUP|POLLNVAL;
		pfd[npfd].revents=0;
		pfd[npfd++].fd=icmp_sock;
	}
	if (icmp6_sock){
		pfd[npfd].events=POLLIN|POLLERR|POLLHUP|POLLNVAL;
		pfd[npfd++].fd=icmp6_sock;
		pfd[npfd].revents=0;
	}
	while(!signal_received){
		gettimeofday(&cur_time,NULL);
		if ( !timercmp(&next_probe,&cur_time,>) )
			timerclear(&next_probe);
		if (foreground)
			debug("%s",ctime(&cur_time.tv_sec));
		for(t=targets;t;t=t->next){
			if ( t->config.alarmdown>0 && !t->alarm_down
				&& (t->last_sent - t->last_received > t->config.alarmdown)){
				t->alarm_down=1;
				toggle_alarm(t,"down",1);
			}
			if (timercmp(&(t->next_probe),&cur_time,<)){
				send_probe(t,&cur_time);
			}
			if (!timerisset(&next_probe) || timercmp(&t->next_probe,&next_probe,<))
				next_probe=t->next_probe;
		}
		debug("Next event scheduled for %s",ctime(&next_probe.tv_sec));
		gettimeofday(&cur_time,NULL);
		if (timercmp(&next_probe,&cur_time,<)){
			timeout=0;
		}
		else{
			timersub(&next_probe,&cur_time,&tv);
			timeout=tv.tv_usec/1000+tv.tv_sec*1000;
			if (timeout>MAX_POLL_TIMEOUT)
				timeout=MAX_POLL_TIMEOUT;
		}
		debug("Polling, timeout: %5.3fs",((double)timeout)/1000);
		poll(pfd,npfd,timeout);
		for(i=0;i<npfd;i++){
			if (!pfd[i].revents&POLLIN) continue;
			if (pfd[i].fd==icmp_sock) recv_icmp();
			else if (pfd[i].fd==icmp6_sock) recv_icmp6();
			pfd[i].revents=0;
		}
	}
}

void usage(const char *name){
	fprintf(stderr,"Alarm Pinger 1.0 (c) 2002 Jacek Konieczny <jajcus@pld.org.pl>\n");
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s [OPTIONS]\n",name);
	fprintf(stderr,"\nOPTIONS:\n");
	fprintf(stderr,"\t-f\trun in foreground.\n");
	fprintf(stderr,"\t-d\tdebug on.\n");
	fprintf(stderr,"\t-h\tthis message.\n");
}

int main(int argc,char *argv[]){
struct passwd *pw;
int c;
FILE *pidfile;
pid_t pid;
int i;

	while((c=getopt(argc,argv,"fdh")) != -1){
		switch(c){
			case 'f':
				foreground=1;
				break;
			case 'd':
				debug_on=1;
				break;
			case 'h':
				usage(argv[0]);
				return 1;
			case '?':
				if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
				        fprintf (stderr, "Unknown option character `\\x%x'.\n",
							optopt);
				return 1;
			default:
				return 1;
		}
	}
	if (!foreground){
		pidfile=fopen(PID_FILE,"r");
		if (pidfile){
			fscanf(pidfile,"%d",&pid);
			if (pid>0 && kill(pid,0)==0){
				fprintf(stderr,"pinger already running\n");
				return 1;
			}
			fclose(pidfile);
		}
		pidfile=fopen(PID_FILE,"w");
		if (!pidfile){
			fprintf(stderr,"Couldn't open pid file.");
			perror(PID_FILE);
			return 1;
		}
	}

	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmp_sock<0){
		myperror("socket");
	}
	icmp6_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (icmp6_sock>=0){
		int err, csum_offset, sz_opt;
		csum_offset = 2;
		sz_opt = sizeof(int);
		err = setsockopt(icmp6_sock, SOL_RAW, IPV6_CHECKSUM, &csum_offset, sz_opt);
		if (err < 0) {
			myperror("setsockopt(IPV6_CHECKSUM)");
			return 1;
		}
	}
	if (icmp6_sock<0 && icmp_sock<0){
		myperror("socket");
		return 1;
	}
	pw=getpwnam("pinger");
	if (!pw) {
		debug("getpwnam(\"pinger\") failed.");
		return 1;
	}
	if (initgroups("pinger",pw->pw_gid)){
		myperror("initgroups");
		return 1;
	}
	if (setgid(pw->pw_gid)){
		myperror("setgid");
		return 1;
	}
	if (setuid(pw->pw_uid)){
		myperror("setuid");
		return 1;
	}

	read_config();

	if (!foreground){
		pid=fork();
		if (pid<0){
			perror("fork");
			fclose(pidfile);
			exit(1);
		}
		if (pid>0){ /* parent */
			fprintf(pidfile,"%i\n",pid);
			fclose(pidfile);
			exit(0);
		}
		for(i=0;i<255;i++)
			if (i!=icmp_sock && i!=icmp6_sock)
				close(i);
		setsid();	
	}
	
	ident=getpid();
	signal(SIGTERM,signal_handler);
	signal(SIGINT,signal_handler);
	signal(SIGPIPE,SIG_IGN);
	if (icmp_sock>=0) install_filter();
	if (icmp6_sock>=0) install_filter6();
	main_loop();
	if (icmp_sock>=0) close(icmp_sock);
	if (icmp6_sock>=0) close(icmp6_sock);

	log("Exiting on signal %i.",signal_received);

	return 0;
}

