#include <assert.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "conf.h"

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
	p=(char *)pool_malloc(pool,strlen(str));
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

