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
 *  $Id: apinger.c,v 1.19 2002/07/18 12:32:30 cvs-jajcus Exp $
 */

#include "config.h"
#include "apinger.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

#include "debug.h"
#include "tv_macros.h"

#ifdef HAVE_ASSERT_H
# include <assert.h>
#else
# define assert(cond)
#endif

struct timeval operation_started;

int is_alarm_on(struct target *t,struct alarm_cfg *a){
struct alarm_list *al;

	for(al=t->active_alarms;al;al=al->next)
		if (al->alarm==a)
			return 1;
	return 0;
}

unsigned alarm_on(struct target *t,struct alarm_cfg *a){
struct alarm_list *al;
struct timeval tv;

	gettimeofday(&tv,NULL);
	al=(struct alarm_list *)malloc(sizeof(struct alarm_list));
	al->next=t->active_alarms;
	al->alarm=a;
	al->id=tv.tv_usec/1000+tv.tv_sec*1000;
	t->active_alarms=al;
	return al->id;
}

unsigned alarm_off(struct target *t,struct alarm_cfg *a){
struct alarm_list *al,*pa,*na;
int id;

	pa=NULL;
	for(al=t->active_alarms;al;al=na){
		na=al->next;
		if (al->alarm==a){
			if (pa!=NULL)
				pa->next=na;
			else
				t->active_alarms=na;
			id=al->id;
			free(al);
			return id;
		}
		else pa=al;
	}
	logit("Alarm '%s' not found in '%s'",a->name,t->name);
	return 0;
}

static char *macros_buf=NULL;
static int macros_buf_l=0;
const char * subst_macros(const char *string,struct target *t,struct alarm_cfg *a,int on){
char *p;
int nmacros=0;
int i,sl,l,n;
char **values;
char ps[16],pr[16],al[16],ad[16],ts[100];
time_t tim;

	if (string==NULL || string[0]=='\000') return "";
	for(i=0;string[i]!='\000';i++){
		if (string[i]!='%') continue;
		nmacros++;
		i++;
		if (string[i]=='\000') break;
	}
	if (nmacros==0) return string;
	values=(char **)malloc(sizeof(char *)*(nmacros+1));
	assert(values!=NULL);
	l=sl=strlen(string);
	n=0;
	for(i=0;i<sl;i++){
		if (string[i]!='%') continue;
		i++;
		switch(string[i]){
		case 't':
			values[n]=t->name;
			break;
		case 'T':
			values[n]=t->config->description;
			break;
		case 'a':
			values[n]=a->name;
			break;
		case 'A':
			switch(a->type){
			case AL_DOWN:
				values[n]="down";
				break;
			case AL_LOSS:
				values[n]="loss";
				break;
			case AL_DELAY:
				values[n]="delay";
				break;
			default:
				values[n]="unknown";
				break;
			}
			break;
		case 'r':
			switch(on){
			case -1:
				values[n]="alarm cancelled (config reload)";
				break;
			case 0:
				values[n]="alarm cancelled";
				break;
			default:
				values[n]="ALARM";
				break;
			}
			break;
		case 'p':
			sprintf(ps,"%i",t->last_sent);
			values[n]=ps;
			break;
		case 'P':
			sprintf(pr,"%i",t->received);
			values[n]=pr;
			break;
		case 'l':
			if (t->upsent > t->config->avg_loss_delay_samples
						+t->config->avg_loss_samples){
				sprintf(al,"%0.1f%%",
						100*((double)t->recently_lost)/
							t->config->avg_loss_samples);
				values[n]=al;
			}
			else values[n]="n/a";
			break;
		case 'd':
			if (t->upsent > t->config->avg_loss_delay_samples
						+t->config->avg_loss_samples){
				sprintf(ad,"%0.2fms",t->delay_sum/t->config->avg_delay_samples);
				values[n]=ad;
			}
			else values[n]="n/a";
			break;
		case 's':
			tim=time(NULL);
			strftime(ts,100,config->timestamp_format,localtime(&tim));
			values[n]=ts;
			break;
		case '%':
			values[n]="%";
			break;
		default:
			values[n]="";
			break;
		}
		l+=strlen(values[n])-1;
		n++;
	}
	values[n]=NULL;
	l+=2;
	if (macros_buf == NULL){
		macros_buf=(char *)malloc(l);
		macros_buf_l=l;
	}else if (macros_buf_l < l){
		macros_buf=(char *)realloc(macros_buf,l);
		macros_buf_l=l;
	}
	assert(macros_buf!=NULL);
	p=macros_buf;
	n=0;
	for(i=0;i<sl;i++){
		if (string[i]!='%'){
			*p++=string[i];
			continue;
		}
		strcpy(p,values[n]);
		p+=strlen(values[n]);
		n++;
		i++;
	}
	free(values);
	*p='\000';
	return macros_buf;
}

void write_report(FILE *f,struct target *t,struct alarm_cfg *a,int on){
time_t tm;
	
	tm=time(NULL);
	fprintf(f,"%s",ctime(&tm));
	if (on)
		fprintf(f,"ALARM went off: %s\n",a->name);
	else
		fprintf(f,"alarm cancelled: %s\n",a->name);
	fprintf(f,"Target: %s\n",t->name);
	fprintf(f,"Description: %s\n",t->config->description);
	fprintf(f,"Probes sent: %i\n",t->last_sent+1);
	fprintf(f,"Replies received: %i\n",t->received);
	fprintf(f,"Last reply received: #%i %s",t->last_received,
			ctime(&t->last_received_tv.tv_sec));
	if (t->received>=t->config->avg_delay_samples){
		fprintf(f,"Recent avg. delay: %4.2fms\n",
				t->delay_sum/t->config->avg_delay_samples);
	}
	if (t->upsent>t->config->avg_loss_delay_samples+t->config->avg_loss_samples){
		fprintf(f,"Recent avg. packet loss: %5.1f%%\n",
				100*((double)t->recently_lost)/t->config->avg_loss_samples);
	}
}

void toggle_alarm(struct target *t,struct alarm_cfg *a,int on){
FILE *p;
char buf[1024];
int ret;
char *mailto,*mailfrom,*mailenvfrom;
const char *command;
unsigned thisid,lastid;

	if (on>0){
		logit("ALARM: %s(%s)  *** %s ***",t->config->description,t->name,a->name);
		thisid=alarm_on(t,a);
	}
	else{
		lastid=alarm_off(t,a);
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
			snprintf(buf,1024,"%s -f'%s'",config->mailer,mailenvfrom);
		}
		else{
			snprintf(buf,1024,"%s",config->mailer);
		}
		debug("Popening: %s",buf);
		p=popen(buf,"w");
		if (!p){
			myperror("Couldn't send mail, popen:");
			return;
		}
		fprintf(p,"Subject: %s\n",subst_macros(a->mailsubject,t,a,on));
		fprintf(p,"To: %s\n",subst_macros(mailto,t,a,on));
		if (mailfrom) {
			fprintf(p,"From: %s\n",subst_macros(mailfrom,t,a,on));
		}
		gethostname(buf,1024);
		fprintf(p,"Message-Id: <apinger-%u-%s-%s-%s@%s>\n",
				thisid,t->name,a->name,(on>0)?"on":"off",buf);
		if (on<=0)
			fprintf(p,"References: <apinger-%u-%s-%s-%s@%s>\n",
				lastid,t->name,a->name,"on",buf);
				
		fprintf(p,"\n");
		write_report(p,t,a,on);
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
	if (on>0) command=a->pipe_on;
	else command=a->pipe_off;
	if (command){
		command=subst_macros(command,t,a,on);
		debug("Popening: %s",command);
		p=popen(command,"w");
		if (!p){
			logit("Couldn't pipe report through %s",command);
			myperror("popen");
			return;
		}
		write_report(p,t,a,on);
		ret=pclose(p);
		if (!WIFEXITED(ret)){
			logit("Error while piping report.");
			logit("command (%s) terminated abnormally.",command);
			return;
		}
		if (WEXITSTATUS(ret)!=0){
			logit("Error while piping report.");
			logit("command (%s) exited with status: %i",command,WEXITSTATUS(ret));
			return;
		}
	}
	if (on>0) command=a->command_on;
	else command=a->command_off;
	if (command){
		command=subst_macros(command,t,a,on);
		debug("Starting: %s",command);
		ret=system(command);
		if (!WIFEXITED(ret)){
			logit("Error while piping report.");
			logit("command (%s) terminated abnormally.",command);
			return;
		}
		if (WEXITSTATUS(ret)!=0){
			logit("Error while piping report.");
			logit("command (%s) exited with status: %i",command,WEXITSTATUS(ret));
			return;
		}
	}
}

void send_probe(struct target *t,struct timeval *cur_time){
int i,i1;
char buf[100];
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

	t->upsent++;
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
	
	if (t->upsent>t->config->avg_loss_delay_samples+t->config->avg_loss_samples){
		avg_loss=100*((double)t->recently_lost)/t->config->avg_loss_samples;
	}else
		avg_loss=0;
	debug("(avg. loss: %5.1f%%)",avg_loss);
	
	pa=NULL;
	for(al=t->active_alarms;al;al=na){
		na=al->next;
		a=al->alarm;
		if (a->type==AL_DOWN){
			t->upsent=0;
			avg_loss=0;
		}
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
		case AL_LOSS:
			if ( avg_loss > a->p.lh.high )
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
	gettimeofday(&operation_started,NULL);
}

void reload_config(void){
struct target *t;
struct alarm_list *al,*an;
struct alarm_cfg *a;
int r;
	
	for(t=targets;t;t=t->next)
		for(al=t->active_alarms;al;al=an){
			an=al->next;
			a=al->alarm;
			toggle_alarm(t,a,-1);
		}
	r=load_config(config_file);
	if (r==0) configure_targets();
}

void write_status(void){
FILE *f;
struct target *t;
struct alarm_list *al;
struct alarm_cfg *a;

	if (config->status_file==NULL) return;
	
	f=fopen(config->status_file,"w");
	if (f==NULL){
		logit("Couldn't open status file");
		myperror(config->status_file);
		return;
	}
	for(t=targets;t;t=t->next){
		fprintf(f,"Target: %s\n",t->name);
		fprintf(f,"Description: %s\n",t->config->description);
		if (t->received>=t->config->avg_delay_samples){
			fprintf(f,"Average delay: %0.2fms\n",
				t->delay_sum/t->config->avg_delay_samples);
		}
		if (t->upsent>t->config->avg_loss_delay_samples+t->config->avg_loss_samples){
			fprintf(f,"Average packet loss: %0.1f%%\n",
				100*((double)t->recently_lost)/t->config->avg_loss_samples);
		}
		fprintf(f,"Active alarms:");
		if (t->active_alarms){
			for(al=t->active_alarms;al;al=al->next){
				a=al->alarm;
				fprintf(f," \"%s\"",a->name);
			}
			fprintf(f,"\n");
		}
		else fprintf(f," None\n");
		fprintf(f,"\n");
	}
	fclose(f);
}

void main_loop(void){
struct target *t;
struct timeval cur_time,next_status,tv;
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
	if (config->status_interval){
		gettimeofday(&cur_time,NULL);
		tv.tv_sec=config->status_interval/1000;
		tv.tv_usec=config->status_interval*1000;
		timeradd(&cur_time,&tv,&next_status);
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
				if (timerisset(&t->last_received_tv)){
					timersub(&cur_time,&t->last_received_tv,&tv);
				}
				else {
					timersub(&cur_time,&operation_started,&tv);
				}
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
		if (config->status_interval){
			if (timercmp(&next_status,&cur_time,<)){
				tv.tv_sec=config->status_interval/1000;
				tv.tv_usec=config->status_interval*1000;
				timeradd(&cur_time,&tv,&next_status);
				if (config->status_file) write_status();
				status_request=0;
			}
			if (!timerisset(&next_probe) || timercmp(&next_status,&next_probe,<))
				next_probe=next_status;
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
		if (status_request){
			status_request=0;
			if (config->status_file){
				logit("SIGUSR1 received, writting status.");
				write_status();
			}
		}
		if (reload_request){
			reload_request=0;
			logit("SIGHUP received, reloading configuration.");
			reload_config();
		}
	}
}


