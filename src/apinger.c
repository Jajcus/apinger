#include <time.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/poll.h>
#include "apinger.h"

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

void send_probe(struct target *t,struct timeval *cur_time){
int i,i1;
double avg_loss;
char buf[100];

	timeradd(cur_time,&t->interval_tv,&t->next_probe);
	debug("Sending ping to %s (%s)",t->desc,t->name);
	strftime(buf,100,"%b %d %H:%M:%S",localtime(&t->next_probe.tv_sec));
	debug("Next one scheduled for %s",buf);
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


void analyze_reply(struct timeval time_recv,int icmp_seq,struct trace_info *ti){
struct target *t;
struct timeval tv;
double delay,avg_delay,avg_loss;
double tmp;
int i;
int previous_received;

	if (icmp_seq!=(ti->seq%65536)){
		debug("Sequence number mismatch.");
		return;
	}
		
	for(t=targets;t!=NULL;t=t->next){
		if (t==ti->target_id) break;
	}
	if (t==NULL){
		log("Couldn't match any target to the echo reply.\n");
		return;
	}
	previous_received=t->last_received;
	if (ti->seq>t->last_received) t->last_received=ti->seq;
	timersub(&time_recv,&ti->timestamp,&tv);
	delay=tv.tv_sec*1000.0+((double)tv.tv_usec)/1000.0;
	debug("#%i from %s(%s) delay: %4.2fms",ti->seq,t->desc,t->name,delay);
	if (t->alarm_down && ti->seq > previous_received){
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

	i=ti->seq%(AVG_LOSS_DELAY_SAMPLES+AVG_LOSS_SAMPLES);
	if (!t->queue[i] && ti->seq<t->last_sent-AVG_LOSS_DELAY_SAMPLES)
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

void main_loop(void){
struct target *t;
struct timeval cur_time,tv;
struct pollfd pfd[2];
int timeout;
int npfd=0;
int i;
char buf[100];	

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
		strftime(buf,100,"%b %d %H:%M:%S",localtime(&next_probe.tv_sec));
		debug("Next event scheduled for %s",buf);
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


