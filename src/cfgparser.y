
%{
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "conf.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

void yyerror(const char *s);
int yylex(void);

extern FILE *yyin, *yyout;
struct config config={NULL,NULL,NULL,NULL,NULL,0};
struct alarm_cfg *cur_alarm;
struct target_cfg *cur_target;

void add_alarm(enum alarm_type type);
void add_target(void);
struct alarm_list *alarm2list(const char *aname,struct alarm_list *list);

%}

%union {
	int i;
	char *s;
	struct alarm_list *al;
}

%token <i> TIME
%token <i> INTEGER
%token <i> BOOLEAN
%token <s> STRING

%token DEBUG
%token ALARM
%token TARGET

%token DEFAULT

%token MAILTO
%token MAILFROM
%token MAILENVFROM

%token DOWN
%token LOSS
%token DELAY

%token SAMPLES
%token PERCENT_LOW
%token PERCENT_HIGH
%token DELAY_LOW
%token DELAY_HIGH

%token DESCRIPTION
%token ALARMS
%token INTERVAL

%type <s> string
%type <al> alarmlist

%expect 12

%%

config:	/* */
	| DEBUG BOOLEAN { config.debug=$2; }
	| alarm 
	| target 
	| config separator config
;

makealarm: 	{ 
	 		struct pool_item *p;
	 		cur_alarm=(struct alarm_cfg*)malloc(sizeof(struct alarm_cfg));
			memset(cur_alarm,0,sizeof(struct alarm_cfg));
			p=(struct pool_item*)malloc(sizeof(struct pool_item));
			p->ptr=cur_alarm;
			p->next=config.pool;
		}
;

maketarget: 	{ 
	 		struct pool_item *p;
	 		cur_target=(struct target_cfg*)malloc(sizeof(struct target_cfg));
			memset(cur_target,0,sizeof(struct target_cfg));
			p=(struct pool_item*)malloc(sizeof(struct pool_item));
			p->ptr=cur_target;
			p->next=config.pool;
		}
;

alarm:	ALARM makealarm DEFAULT '{' alarmcommoncfg '}' 
     		{ 
			cur_alarm->name="default";
			config.alarm_defaults=cur_alarm; 
		}
     	| ALARM makealarm DOWN string '{' alarmdowncfg '}' 
		{ 
			cur_alarm->name=$4;
			add_alarm(AL_DOWN); 
		}
     	| ALARM makealarm LOSS string '{' alarmlosscfg '}' 
		{ 
			cur_alarm->name=$4;
			add_alarm(AL_LOSS); 
		}
	| ALARM makealarm DELAY string '{' alarmdelaycfg '}' 
		{ 
			cur_alarm->name=$4;
			add_alarm(AL_DELAY); 
		}
;

alarmcommoncfg: alarmcommon
	| alarmcommoncfg separator alarmcommoncfg
;

alarmlosscfg: alarmcommon
	| PERCENT_LOW INTEGER 
		{ cur_alarm->p.lh.low=$2; }
	| PERCENT_HIGH INTEGER 
		{ cur_alarm->p.lh.low=$2; }
	| alarmlosscfg separator alarmlosscfg
;

alarmdelaycfg: alarmcommon
	| DELAY_LOW TIME 
		{ cur_alarm->p.lh.low=$2; }
	| DELAY_HIGH TIME 
		{ cur_alarm->p.lh.high=$2; }
	| alarmdelaycfg separator alarmdelaycfg
;

alarmdowncfg: alarmcommon
	| SAMPLES INTEGER 
		{ cur_alarm->p.val=$2; }
	| alarmdowncfg separator alarmdowncfg
;

alarmcommon: /* */ 
	| MAILTO string	
		{ cur_alarm->mailto=$2; }
	| MAILFROM string 
		{ cur_alarm->mailfrom=$2; }
	| MAILENVFROM string 
		{ cur_alarm->mailenvfrom=$2; }
;


target:	TARGET maketarget DEFAULT '{' targetcfg '}' 
      		{ 
			cur_target->name="default";
			config.target_defaults=cur_target; 
		}
     	| TARGET maketarget string '{' targetcfg '}' 
		{ 
			cur_target->name=$3;
			add_target(); 
		}
;

targetcfg: /* */ 
	| DESCRIPTION string 
		{ cur_target->description=$2; }
	| ALARMS alarmlist
		{ cur_target->alarms=$2; }
	| INTERVAL INTEGER
		{ cur_target->interval=$2; }
	| targetcfg separator targetcfg
;

alarmlist: string
		{ $$=alarm2list($1,NULL); }
	| alarmlist ',' string 
		{ $$=alarm2list($3,$1); }
;

string: STRING	{ char *s; struct pool_item *p;
      			s=strdup($1);
			p=(struct pool_item*)malloc(sizeof(struct pool_item));
			p->ptr=s;
			p->next=config.pool;
			$$=s;
		}
;

separator: '\n'
	| ';'
;


%%
void yyerror (const char *s) {
	printf ("%s\n", s);
}

void add_alarm(enum alarm_type type){
	cur_alarm->type=type;
	cur_alarm->next=config.alarms;
	config.alarms=cur_alarm;
	cur_alarm=NULL;
}

void add_target(void){
	cur_target->next=config.targets;
	config.targets=cur_target;
	cur_target=NULL;
}

struct alarm_list *alarm2list(const char *aname,struct alarm_list *list){
struct alarm_cfg *ac;
struct alarm_list *al; 
struct pool_item *p;

	for(ac=config.alarms;ac!=NULL;ac=ac->next)
		if (strcmp(ac->name,aname)==0) break;
	if (ac==NULL){
		fprintf(stderr,"Alarm '%s' not found.\n",aname);
		return list;
	}
	al=malloc(sizeof(struct alarm_list));
	p=malloc(sizeof(struct pool_item));
	p->ptr=al;
	p->next=config.pool;
	al->alarm=ac;
	al->next=list;
	return al;
}

int main( int argc, char *argv[] ){
int ret;

	yyin=fopen("/etc/apinger.conf","r");
	yydebug=1;
	ret=yyparse();
	return 0;
}
