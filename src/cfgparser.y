
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
struct config cur_config={NULL,NULL,NULL,NULL,NULL,0};
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

%token ERROR

%type <s> string
%type <al> alarmlist
%type <a> makealarm
%type <t> maketarget

%%

config:	/* */
	| DEBUG BOOLEAN { cur_config.debug=$2; }
	| alarm 
	| target 
	| config separator config
;

makealarm: 	{ $$=make_alarm(); } 
;

maketarget: 	{ $$=make_target(); }
;

alarm:	ALARM makealarm DEFAULT '{' alarmcommoncfg '}' 
     		{ 
			cur_alarm->name="default";
			cur_config.alarm_defaults=cur_alarm; 
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
			cur_config.target_defaults=cur_target; 
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

string: STRING	{ $$=pool_strdup(&cur_config.pool,$1); }
;

separator: '\n'
	| ';'
;


%%
void yyerror (const char *s) {
	printf ("%s\n", s);
}

int main( int argc, char *argv[] ){
int ret;

	yyin=fopen("/etc/apinger.conf","r");
	yydebug=0;
	memset(&yylloc,0,sizeof(yylloc));
	ret=yyparse();
	return 0;
}
