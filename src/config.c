#include <arpa/inet.h>
#include <malloc.h>
#include <assert.h>
#include <stdlib.h>
#include "apinger.h"

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
			cf_debug=1;
		}
		else{
			log("Unknown parameter '%s' in %s, ignoring.\n",param,CONFIG);
		}
	}
	fclose(f);
}


