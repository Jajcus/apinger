#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "conf.h"
#include "cfgparser.tab.h"

struct config * config=NULL;

void *pool_malloc(struct pool_item **pool,size_t size){
struct pool_item *pi;
char *p;

	p=(char *)malloc(size+sizeof(struct pool_item));
	assert(p!=NULL);
	pi=(struct pool_item *)p;
	p+=sizeof(struct pool_item);
	pi->next=*pool;
	*pool=pi;
	return p;
}

char *pool_strdup(struct pool_item **pool,const char *str){
char *p;

	assert(str!=NULL);
	p=(char *)pool_malloc(pool,strlen(str)+1);
	strcpy(p,str);
	return p;
}

void pool_free(struct pool_item **pool,void *ptr){
struct pool_item *pi,*pi1;

	pi1=NULL;
	for(pi=*pool;pi;pi=pi->next){
		if (pi+1==ptr){
			if (pi1){
				pi1->next=pi->next;
			}
			else{
				*pool=pi->next;
			}
			free(pi);
			return;
		}
		pi1=pi;
	}
	fprintf(stderr,"poll_free: pointer not from the pool\n");
	abort(1);
}

void pool_clear(struct pool_item **pool){
struct pool_item *pi,*pi1;

	for(pi=*pool;pi;pi=pi1){
		pi1=pi->next;
		free(pi);
	}
}

struct alarm_cfg * make_alarm(){
	cur_alarm=(struct alarm_cfg*)pool_malloc(&cur_config.pool,
							sizeof(struct alarm_cfg));
	memset(cur_alarm,0,sizeof(struct alarm_cfg));
	return cur_alarm;
}

struct target_cfg * make_target(){

	cur_target=(struct target_cfg*)pool_malloc(&cur_config.pool,
						sizeof(struct target_cfg));
	memset(cur_target,0,sizeof(struct target_cfg));
	return cur_target;
}

void add_alarm(enum alarm_type type){
	cur_alarm->type=type;
	cur_alarm->next=cur_config.alarms;
	cur_config.alarms=cur_alarm;
	cur_alarm=NULL;
}

void add_target(void){
	cur_target->next=cur_config.targets;
	cur_config.targets=cur_target;
	cur_target=NULL;
}

struct alarm_list *alarm2list(const char *aname,struct alarm_list *list){
struct alarm_cfg *ac;
struct alarm_list *al; 

	for(ac=cur_config.alarms;ac!=NULL;ac=ac->next)
		if (strcmp(ac->name,aname)==0) break;
	if (ac==NULL){
		fprintf(stderr,"Alarm '%s' not found.\n",aname);
		return list;
	}
	al=pool_malloc(&cur_config.pool,sizeof(struct alarm_list));
	al->alarm=ac;
	al->next=list;
	return al;
}

extern FILE *yyin, *yyout;
extern int yydebug;
extern YYLTYPE yylloc;
int yyparse(void);

int load_config(const char *filename){
struct target_cfg *t;
struct alarm_cfg *a;
struct alarm_list *al;	
int ret;

	yyin=fopen(filename,"r");
	yydebug=0;
	memset(&yylloc,0,sizeof(yylloc));
	cur_config=default_config;	
	ret=yyparse();
	if (ret==0){
		for(a=cur_config.alarms;a;a=a->next){
			if (a->mailto==NULL)
				a->mailto=cur_config.alarm_defaults.mailto;
			if (a->mailfrom==NULL)
				a->mailfrom=cur_config.alarm_defaults.mailfrom;
			if (a->mailenvfrom==NULL)
				a->mailenvfrom=cur_config.alarm_defaults.mailenvfrom;
		}
		for(t=cur_config.targets;t;t=t->next){
			if (t->description==NULL)
				t->description=cur_config.target_defaults.description;
			if (t->interval<=0)
				t->interval=cur_config.target_defaults.interval;
			if (t->avg_delay_samples<=0)
				t->avg_delay_samples=cur_config.target_defaults.avg_delay_samples;
			if (t->avg_loss_samples<=0)
				t->avg_loss_samples=cur_config.target_defaults.avg_loss_samples;
			if (t->avg_loss_delay_samples<=0)
				t->avg_loss_delay_samples=cur_config.target_defaults.avg_loss_delay_samples;
			for(al=t->alarms;al && al->next;al=al->next);
			if (al)
				al->next=cur_config.target_defaults.alarms;
			else
				t->alarms=cur_config.target_defaults.alarms;
		}
		if (config!=NULL){
			struct pool_item *pool=config->pool;
			pool_clear(&pool);
			config=NULL;
		}
	
		config=(struct config *)pool_malloc(&cur_config.pool,sizeof(struct config));
		memcpy(config,&cur_config,sizeof(struct config));
	}
	memset(&cur_config,0,sizeof(cur_config));
	return ret;
}

