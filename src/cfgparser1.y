
%{
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "conf.h"
#include "debug.h"

#define YYDEBUG 1
#define YYERROR_VERBOSE 1

void yyerror(const char *s);
int yylex(void);

extern FILE *yyin, *yyout;
struct config cur_config;
struct alarm_cfg *cur_alarm;
struct target_cfg *cur_target;


%}

%verbose
%locations
%expect 12
%union {
	int i;
	char *s;
	struct alarm_cfg *a;
	struct target_cfg *t;
	struct config *c;
	struct alarm_list *al;
}

%token <i> TIME
%token <i> INTEGER
%token <i> BOOLEAN
%token <s> STRING

%token DEBUG
%token USER
%token GROUP
%token ALARM
%token TARGET

%token DEFAULT

%token MAILTO
%token MAILFROM
%token MAILENVFROM

%token DOWN
%token LOSS
%token DELAY

%token TIME_
%token PERCENT_LOW
%token PERCENT_HIGH
%token DELAY_LOW
%token DELAY_HIGH

%token DESCRIPTION
%token ALARMS
%token INTERVAL
%token AVG_DELAY_SAMPLES
%token AVG_LOSS_SAMPLES
%token AVG_LOSS_DELAY_SAMPLES

%token ERROR

%type <s> string
%type <al> alarmlist
%type <a> makealarm getdefalarm
%type <t> maketarget getdeftarget

%%

config:	/* */
	| DEBUG BOOLEAN { cur_config.debug=$2; }
	| USER string { cur_config.user=$2; }
	| GROUP string { cur_config.group=$2; }
	| alarm 
	| target 
	| config separator config
	| error
		{
			log("Configuration file syntax error. Line %i, character %i",
					@$.first_line+1,@$.first_column+1);
			YYABORT;
		}
;

makealarm: 	{ $$=make_alarm(); } 
;

maketarget: 	{ $$=make_target(); }
;

getdefalarm: 	{ 
	   		$$=&cur_config.alarm_defaults; 
			cur_alarm=$$;
	   	} 
;

getdeftarget: 	{ 
	    		$$=&cur_config.target_defaults; 
			cur_target=$$;
		}
;

alarm:	ALARM getdefalarm DEFAULT '{' alarmcommoncfg '}' 
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
		{ cur_alarm->p.lh.high=$2; }
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
	| TIME_ TIME 
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


target:	TARGET getdeftarget DEFAULT '{' targetcfg '}' 
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
	| INTERVAL TIME
		{ cur_target->interval=$2; }
	| AVG_DELAY_SAMPLES INTEGER
		{ cur_target->avg_delay_samples=$2; }
	| AVG_LOSS_SAMPLES INTEGER
		{ cur_target->avg_loss_samples=$2; }
	| AVG_LOSS_DELAY_SAMPLES INTEGER
		{ cur_target->avg_loss_delay_samples=$2; }
	| targetcfg separator targetcfg
;

alarmlist: string
		{ $$=alarm2list($1,NULL); }
	| alarmlist ',' string 
		{ $$=alarm2list($3,$1); }
;

string: STRING	{ $$=pool_strdup(&cur_config.pool,$1); }
;

separator: '\n'
	| ';'
;


%%
void yyerror (const char *s) {
	log("%s", s);
}

