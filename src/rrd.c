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
 *  $Id: rrd.c,v 1.3 2002/10/16 08:19:38 cvs-jajcus Exp $
 */

#include "config.h"
#include "apinger.h"
#include "rrd.h"
#include "debug.h"

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

#define RRDTOOL_RESTART_TIME (60*5)

FILE *rrdtool_pipe=NULL;
time_t last_rrdtool_start=0;
time_t rrdtool_waiting=0;

int rrd_init(void){
time_t cur_t;

	if (rrdtool_pipe)
		pclose(rrdtool_pipe);
	rrdtool_pipe=NULL;
	if (!config->rrd_interval){
		return -1;
	}
	cur_t=time(NULL);
	if (cur_t-last_rrdtool_start<RRDTOOL_RESTART_TIME){
		if (!rrdtool_waiting)
			logit(RRDTOOL " respawning too fast, waiting %is.",RRDTOOL_RESTART_TIME);
		rrdtool_waiting=1;
		return -1;
	}
	rrdtool_waiting=0;
	last_rrdtool_start=cur_t;
	rrdtool_pipe=popen(RRDTOOL " - >/dev/null 2>&1","w");
	if (rrdtool_pipe==NULL){
		myperror(RRDTOOL);
		return -1;
	}
#if defined(HAVE_SETVBUF) && defined(_IOLBF)
	setvbuf(rrdtool_pipe, (char *)NULL, _IOLBF, 0);
#endif
	return 0;	
}

void rrd_create(void){
struct target *t;
const char *filename;
int ret;

	for(t=targets;t!=NULL;t=t->next){
		if (t->config->rrd_filename==NULL) continue;
		filename=subst_macros(t->config->rrd_filename,t,NULL,0);
#if defined(HAVE_ACCESS) && defined(F_OK)
		if (access(filename,F_OK)==0) continue;
#else
		{
			FILE *f;
			f=fopen(filename,"r");
			if (f!=NULL){
				fclose(f);
				continue;
			}
		}
#endif
		if (rrdtool_pipe==NULL) 
			if (rrd_init()) return;
		ret=fprintf(rrdtool_pipe,"create %s"
				" DS:loss:GAUGE:600:0:100"
				" DS:delay:GAUGE:600:0:100000"
				" RRA:AVERAGE:0.5:1:600"
				" RRA:AVERAGE:0.5:6:700"
				" RRA:AVERAGE:0.5:24:775"
				" RRA:AVERAGE:0.5:288:796\n",filename);
		if (ret<0){
			myperror("Error while feeding rrdtool");
			pclose(rrdtool_pipe);
			rrdtool_pipe=NULL;
		}
	}
}

void rrd_update(void){
struct target *t;
const char *filename;
int ret;

	if (sigpipe_received) {
		sigpipe_received=0;
		if (rrdtool_pipe) pclose(rrdtool_pipe);
		rrdtool_pipe=NULL;
	}
	for(t=targets;t!=NULL;t=t->next){
		if (t->config->rrd_filename==NULL) continue;
		if (rrdtool_pipe==NULL) 
			if (rrd_init()) return;
		filename=subst_macros(t->config->rrd_filename,t,NULL,0);
		ret=fprintf(rrdtool_pipe,"update %s -t loss:delay N",filename);
		if (ret>0){
			if (t->upsent > t->config->avg_loss_delay_samples
						+t->config->avg_loss_samples){
				ret=fprintf(rrdtool_pipe,":%f",
						100*((double)t->recently_lost)/
							t->config->avg_loss_samples);
			}
			else ret=fprintf(rrdtool_pipe,":U");
		}
		if (ret>0){
			if (t->upsent > t->config->avg_delay_samples){
				fprintf(rrdtool_pipe,":%f",
						(t->delay_sum/t->config->avg_delay_samples)/1000);
			}
			else ret=fprintf(rrdtool_pipe,":U");
		}
		if (ret>0)
			ret=fprintf(rrdtool_pipe,"\n");
		if (ret<0){
			myperror("Error while feeding rrdtool");
			pclose(rrdtool_pipe);
			rrdtool_pipe=NULL;
		}
	}
}

int rrd_print_cgi(const char *graph_dir,const char *graph_location){
struct target_cfg *tc;
struct target t;
const char *rrd_filename,*p;
char *base_filename,*buf,*p1;
char *ebuf;
int num_esc;

	printf("#!" RRDCGI "\n\n");
	printf("<HTML>\n<HEAD>\n<TITLE> Alarm Pinger statistics </TITLE>\n</HEAD>\n");
	printf("<BODY>\n<H1> Alarm Pinger statistics </H1>\n");
	printf("<H2> Daily packet loss and delay summary </H2>\n");
	memset(&t,0,sizeof(t));
	for(tc=config->targets;tc;tc=tc->next){
		if (tc->rrd_filename==NULL) continue;
		t.name=tc->name;
		t.description=tc->description;
		rrd_filename=subst_macros(tc->rrd_filename,&t,NULL,0);
		buf=strdup(rrd_filename);
		base_filename=strrchr(buf,'/');
		if (base_filename!=NULL)
			*base_filename++=0;
		else
			base_filename=buf;
		p1=strrchr(base_filename,'.');
		if (p1!=NULL) *p1=0;
		num_esc=0;
		for(p=rrd_filename;*p;p++)
			if (*p==':' || *p=='\\') num_esc++;
		if (num_esc>0){
			ebuf=NEW(char,strlen(rrd_filename)+num_esc+1);
			p1=ebuf;
			for(p=rrd_filename;*p;p++){
				if (*p==':' || *p=='\\') *p1++='\\';
				*p1++=*p;
			}
			*p1++=0;
			rrd_filename=ebuf;
		}
		else ebuf=NULL;
		printf("<P><RRD::GRAPH %s/%s-delay.png\n",graph_dir,base_filename);
		printf("--imginfo '<IMG SRC=\"%s/%%s\" WIDTH=\"%%lu\" HEIGHT=\"%%lu\">'\n",
						graph_location);
		printf("-a PNG -h 200 -w 800 --lazy -v 'Packet RTT (s)'\n");
		printf("-t 'Packet delay summary for %s (%s)'\n",tc->name,tc->description);
		printf("-s -1d -l 0\n");
		printf("DEF:delay=%s:delay:AVERAGE\n",rrd_filename);
		printf("AREA:delay#00a000:\n");
		printf("LINE1:delay#004000:\n");
		printf("GPRINT:delay:MIN:\"Minimum\\: %%7.3lf%%ss\"\n");
		printf("GPRINT:delay:AVERAGE:\"Average\\: %%7.3lf%%ss\"\n");
		printf("GPRINT:delay:MAX:\"Maximum\\: %%7.3lf%%ss\\j\"\n");
		printf("></P>");
		
		printf("<P><RRD::GRAPH %s/%s-loss.png\n",graph_dir,base_filename);
		printf("--imginfo '<IMG SRC=\"%s/%%s\" WIDTH=\"%%lu\" HEIGHT=\"%%lu\">'\n",
						graph_location);
		printf("-a PNG -h 200 -w 800 --lazy -v 'Packet loss (%%)'\n");
		printf("-t 'Packet loss summary for %s (%s)'\n",tc->name,tc->description);
		printf("-s -1d -l 0 -u 100\n");
		printf("DEF:loss=%s:loss:AVERAGE\n",rrd_filename);
		printf("AREA:loss#f00000:\n");
		printf("LINE1:loss#700000:\n");
		printf("GPRINT:loss:MIN:\"Minimum\\: %%5.1lf%%%%\"\n");
		printf("GPRINT:loss:AVERAGE:\"Average\\: %%5.1lf%%%%\"\n");
		printf("GPRINT:loss:MAX:\"Maximum\\: %%5.1lf%%%%\\j\"\n");
		printf("></P>\n");
		free(buf);
		free(ebuf);
	}
	printf("<P><b>apinger</b> by Jacek Konieczny</P>\n");
	printf("</BODY></HTML>\n");
	return 0;
}

void rrd_close(void){

	if (rrdtool_pipe)
		pclose(rrdtool_pipe);
}
