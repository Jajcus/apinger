#include "config.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#if TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include "apinger.h"
#include "conf.h"
#include "debug.h"
#include "tv_macros.h"

#ifdef HAVE_ASSERT_H
# include <assert.h>
#else
# define assert(cond)
#endif

int is_alarm_on(struct target *t,struct alarm_cfg *a){
struct alarm_list *al;

	for(al=t->active_alarms;al;al=al->next)
		if (al->alarm==a)
			return 1;
	return 0;
}

void alarm_on(struct target *t,struct alarm_cfg *a){
struct alarm_list *al;

	al=(struct alarm_list *)malloc(sizeof(struct alarm_list));
	al->next=t->active_alarms;
	al->alarm=a;
	t->active_alarms=al;
}

void alarm_off(struct target *t,struct alarm_cfg *a){
struct alarm_list *al,*pa,*na;

	pa=NULL;
	for(al=t->active_alarms;al;al=na){
		na=al->next;
		if (al->alarm==a){
			if (pa!=NULL)
				pa->next=na;
			else
				t->active_alarms=na;
			free(al);
			return;
		}
		else pa=al;
	}
	logit("Alarm '%s' not found in '%s'",a->name,t->name);
}

void toggle_alarm(struct target *t,struct alarm_cfg *a,int on){
FILE *p;
char buf[1024];
int ret;
time_t tm;
char *mailto,*mailfrom,*mailenvfrom;


	if (on>0){
		logit("ALARM: %s(%s)  *** %s ***",t->config->description,t->name,a->name);
		alarm_on(t,a);
	}
	else{
		alarm_off(t,a);
		if (on==0)
			logit("alarm cancelled: %s(%s)  *** %s ***",t->config->description,t->name,a->name);
		else
			logit("alarm cancelled (config reload): %s(%s)  *** %s ***",t->config->description,t->name,a->name);
	}

	mailto=a->mailto;
	mailenvfrom=a->mailenvfrom;
	if (strpbrk(mailenvfrom,"\\'")!=0)
		mailenvfrom=NULL;
	mailfrom=a->mailfrom;
	if (mailto){
		if (mailenvfrom){
			snprintf(buf,1024,"/usr/lib/sendmail -t -f'%s'",mailenvfrom);
		}
		else{
			snprintf(buf,1024,"/usr/lib/sendmail");
		}
		p=popen(buf,"w");
		if (!p){
			myperror("Couldn't send mail: popen:");
			return;
		}
		tm=time(NULL);
		if (on>0)
			fprintf(p,"Subject: ALARM: %s(%s) *** %s ***\n",
					t->config->description,t->name,a->name);
		else if (on==0)
			fprintf(p,"Subject: alarm cancelled: %s(%s) *** %s ***\n",
					t->config->description,t->name,a->name);
		else
			fprintf(p,"Subject: alarm cancelled (config reload): %s(%s) *** %s ***\n",
					t->config->description,t->name,a->name);
		fprintf(p,"To: %s\n",mailto);
		if (mailfrom) {
			fprintf(p,"From: Alarm Pinger <%s>\n",mailfrom);
		}
		fprintf(p,"\n%s",ctime(&tm));
		if (on)
			fprintf(p,"ALARM went off: %s\n",a->name);
		else
			fprintf(p,"alarm cancelled: %s\n",a->name);
		fprintf(p,"Target: %s\n",t->name);
		fprintf(p,"Description: %s\n",t->config->description);
		fprintf(p,"Probes sent: %i\n",t->last_sent+1);
		fprintf(p,"Replies received: %i\n",t->received);
		fprintf(p,"Last reply received: %i\n",t->last_received);
		if (t->received>=t->config->avg_delay_samples){
			fprintf(p,"Recent avg. delay: %4.2fms\n",
					t->delay_sum/t->config->avg_delay_samples);
		}
		if (t->last_sent>t->config->avg_loss_delay_samples+t->config->avg_loss_samples){
			fprintf(p,"Recent avg. packet loss: %5.1f%%\n",
					100*((double)t->recently_lost)/t->config->avg_loss_samples);
		}
		ret=pclose(p);
		if (!WIFEXITED(ret)){
			logit("Error while sending mail.\n");
			logit("sendmail terminated abnormally.\n");
			return;
		}
		if (WEXITSTATUS(ret)!=0){
			logit("Error while sending mail.\n");
			logit("sendmail exited with status: %i\n",WEXITSTATUS(ret));
			return;
		}
	}
}

void send_probe(struct target *t,struct timeval *cur_time){
int i,i1;
double avg_loss;
char buf[100];
struct alarm_list *al;
struct alarm_cfg *a;
int seq;

	timeradd(cur_time,&t->interval_tv,&t->next_probe);
	seq=++t->last_sent;
	debug("Sending ping #%i to %s (%s)",seq,t->config->description,t->name);
	strftime(buf,100,"%b %d %H:%M:%S",localtime(&t->next_probe.tv_sec));
	debug("Next one scheduled for %s",buf);
	if (t->addr.addr.sa_family==AF_INET) send_icmp_probe(t,cur_time,seq);
#ifdef HAVE_IPV6
	else if (t->addr.addr.sa_family==AF_INET6) send_icmp6_probe(t,cur_time,seq);
#endif

	if (t->last_sent>=t->config->avg_loss_delay_samples+t->config->avg_loss_samples){
		i=t->last_sent%(t->config->avg_loss_delay_samples+t->config->avg_loss_samples);
		if (!t->queue[i]) t->recently_lost--;
		t->queue[i]=0;
	}

	if (t->last_sent>=t->config->avg_loss_delay_samples){
		i1=(t->last_sent-t->config->avg_loss_delay_samples)
			%(t->config->avg_loss_delay_samples+t->config->avg_loss_samples);
		if (!t->queue[i1]) t->recently_lost++;
		debug("Recently lost packets: %i",t->recently_lost);
	}

	if (t->last_sent>t->config->avg_loss_delay_samples+t->config->avg_loss_samples){
		avg_loss=100*((double)t->recently_lost)/t->config->avg_loss_samples;
		debug("Checking loss alarm conditions for %s (avg. loss: %6.3f)",t->name,avg_loss);
		for(al=t->config->alarms;al;al=al->next){
			a=al->alarm;
			if (a->type!=AL_LOSS || is_alarm_on(t,a)) continue;
			if ( avg_loss > a->p.lh.high ){
				toggle_alarm(t,a,1);
			}
		}
	}
}


void analyze_reply(struct timeval time_recv,int icmp_seq,struct trace_info *ti){
struct target *t;
struct timeval tv;
double delay,avg_delay,avg_loss;
double tmp;
int i;
int previous_received;
struct alarm_list *al,*pa,*na;
struct alarm_cfg *a;

	if (icmp_seq!=(ti->seq%65536)){
		debug("Sequence number mismatch.");
		return;
	}
		
	for(t=targets;t!=NULL;t=t->next){
		if (t==ti->target_id) break;
	}
	if (t==NULL){
		logit("Couldn't match any target to the echo reply.\n");
		return;
	}
	previous_received=t->last_received;
	if (ti->seq>t->last_received) t->last_received=ti->seq;
	t->last_received_tv=time_recv;
	timersub(&time_recv,&ti->timestamp,&tv);
	delay=tv.tv_sec*1000.0+((double)tv.tv_usec)/1000.0;
	debug("#%i from %s(%s) delay: %4.2fms",ti->seq,t->config->description,t->name,delay);
	tmp=t->rbuf[t->received%t->config->avg_delay_samples];
	t->rbuf[t->received%t->config->avg_delay_samples]=delay;
	t->delay_sum+=delay-tmp;
	t->received++;

	if (t->received>t->config->avg_delay_samples){
		avg_delay=t->delay_sum/t->config->avg_delay_samples;
	}
	else avg_delay=t->delay_sum/t->received;
	debug("(avg: %4.2fms)",avg_delay);

	i=ti->seq%(t->config->avg_loss_delay_samples+t->config->avg_loss_samples);
	if (!t->queue[i] && ti->seq<t->last_sent-t->config->avg_loss_delay_samples)
		t->recently_lost--;
	t->queue[i]=1;
	if (t->last_sent>t->config->avg_loss_delay_samples+t->config->avg_loss_samples)
		avg_loss=100*((double)t->recently_lost)/t->config->avg_loss_samples;
	else
		avg_loss=0;
	debug("(avg. loss: %5.1f%%)",avg_loss);

	pa=NULL;
	for(al=t->active_alarms;al;al=na){
		na=al->next;
		a=al->alarm;
		if ((a->type==AL_DOWN)
		   || (a->type==AL_DELAY && avg_delay<a->p.lh.low)
		   || (a->type==AL_LOSS && avg_loss<a->p.lh.low) ){
			toggle_alarm(t,a,0);
		}
	}

	for(al=t->config->alarms;al;al=al->next){
		a=al->alarm;
		if (is_alarm_on(t,a)) continue;
		switch(a->type){
		case AL_DELAY:
			if (t->received>t->config->avg_delay_samples && avg_delay>a->p.lh.high )
					toggle_alarm(t,a,1);
			break;
		default:
			break;
		}
	}
}

void configure_targets(void){
struct target *t,*pt,*nt;
struct target_cfg *tc;
struct alarm_list *al,*nal;
union addr addr;
int r;

	/* delete all not configured targets */
	pt=NULL;
	for(t=targets;t;t=nt){
		for(tc=config->targets;tc;tc=tc->next)
			if (strcmp(tc->name,t->name)==0)
				break;
		nt=t->next;
		if (tc==NULL){
			if (pt==NULL)
				targets=t;
			else
				pt->next=nt;
			for(al=t->active_alarms;al;al=nal){
				nal=al->next;
				free(al);
			}
			free(t->name);
			free(t);
		}
		else pt=t;
	}

	/* Update target configuration */
	for(tc=config->targets;tc;tc=tc->next){
		for(t=targets;t;t=t->next)
			if (!strcmp(t->name,tc->name))
				break;
		if (t==NULL) { /* new target */
			memset(&addr,0,sizeof(addr));
			r=inet_pton(AF_INET,tc->name,&addr.addr4.sin_addr);
			if (r){
				if (icmp_sock<0){
					logit("Sorry, IPv4 is not available\n");
					logit("Ignoring target %s\n",tc->name);
					continue;
				}
				addr.addr.sa_family=AF_INET;
			}else{
#ifdef HAVE_IPV6
				r=inet_pton(AF_INET6,tc->name,&addr.addr6.sin6_addr);
				if (r==0){
#endif
					logit("Bad host address: %s\n",tc->name);
					logit("Ignoring target %s\n",tc->name);
					continue;
#ifdef HAVE_IPV6
				}
				if (icmp6_sock<0){
					logit("Sorry, IPv6 is not available\n");
					logit("Ignoring target %s\n",tc->name);
					continue;
				}
				addr.addr.sa_family=AF_INET6;
#endif
			}
			t=(struct target *)malloc(sizeof(struct target));
			memset(t,0,sizeof(struct target));
			t->name=strdup(tc->name);
			t->addr=addr;
			t->next=targets;
			targets=t;
		}
		t->config=tc;
		t->interval_tv.tv_usec=(tc->interval%1000)*1000;
		t->interval_tv.tv_sec=tc->interval/1000;
		if (t->queue)
			t->queue=(char *)realloc(t->queue,
					sizeof(char)*(tc->avg_loss_delay_samples
							+tc->avg_loss_samples));
		else
			t->queue=(char *)malloc(
					sizeof(char)*(tc->avg_loss_delay_samples
							+tc->avg_loss_samples));
		assert(t->queue!=NULL);
		if (t->rbuf)
			t->rbuf=(double *)realloc(t->rbuf,sizeof(double)*(tc->avg_delay_samples));
		else
			t->rbuf=(double *)malloc(sizeof(double)*(tc->avg_delay_samples));
		assert(t->rbuf!=NULL);
	}
	if (targets==NULL){
		logit("No usable targets found, exiting");
		exit(1);
	}
}

void reload_config(void){
struct target *t;
struct alarm_list *al,*an;
struct alarm_cfg *a;
int r;
	
	logit("SIGHUP received, reloading configuration");
	for(t=targets;t;t=t->next)
		for(al=t->active_alarms;al;al=an){
			an=al->next;
			a=al->alarm;
			toggle_alarm(t,a,-1);
		}
	r=load_config(CONFIG);
	if (r==0) configure_targets();
}

void main_loop(void){
struct target *t;
struct timeval cur_time,tv;
struct pollfd pfd[2];
int timeout;
int npfd=0;
int i;
char buf[100];	
int downtime;
struct alarm_list *al,*nal;
struct alarm_cfg *a;

	configure_targets();
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
	while(!interrupted_by){
		gettimeofday(&cur_time,NULL);
		if ( !timercmp(&next_probe,&cur_time,>) )
			timerclear(&next_probe);
		for(t=targets;t;t=t->next){
			for(al=t->config->alarms;al;al=nal){
				a=al->alarm;
				nal=al->next;
				if (a->type!=AL_DOWN || is_alarm_on(t,a)) continue;
				if (!timerisset(&t->last_received_tv)) continue;
				timersub(&cur_time,&t->last_received_tv,&tv);
				downtime=tv.tv_sec*1000+tv.tv_usec/1000;
				if ( downtime > a->p.val)
					toggle_alarm(t,a,1);
			}
			if (timercmp(&(t->next_probe),&cur_time,<)){
				send_probe(t,&cur_time);
			}
			if (!timerisset(&next_probe) || timercmp(&t->next_probe,&next_probe,<))
				next_probe=t->next_probe;
		}
		strftime(buf,100,"%b %d %H:%M:%S",localtime(&next_probe.tv_sec));
		debug("Next event scheduled for %s",buf);
		gettimeofday(&cur_time,NULL);
		if (timercmp(&next_probe,&cur_time,<)){
			timeout=0;
		}
		else{
			timersub(&next_probe,&cur_time,&tv);
			timeout=tv.tv_usec/1000+tv.tv_sec*1000;
		}
		debug("Polling, timeout: %5.3fs",((double)timeout)/1000);
		poll(pfd,npfd,timeout);
		for(i=0;i<npfd;i++){
			if (!pfd[i].revents&POLLIN) continue;
			if (pfd[i].fd==icmp_sock) recv_icmp();
			else if (pfd[i].fd==icmp6_sock) recv_icmp6();
			pfd[i].revents=0;
		}
		if (reload_request){
			reload_request=0;
			reload_config();
		}
	}
}


