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
 *  $Id: cfgparser1.y,v 1.12 2002/10/24 08:04:50 cvs-jajcus Exp $
 */


%{

#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
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
%expect 14
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
%token <s> STRING

%token DEBUG
%token USER
%token GROUP
%token PID_FILE
%token MAILER
%token TIMESTAMP_FORMAT
%token RRD


%token STATUS
%token ALARM
%token TARGET

%token DEFAULT

%token MAILTO
%token MAILFROM
%token MAILENVFROM
%token MAILSUBJECT
%token COMMAND
%token PIPE
%token COMBINE
%token REPEAT

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

%token FILE_

%token ERROR

%token ON OFF
%token YES NO
%token TRUE FALSE

%type <s> string
%type <al> alarmlist
%type <a> makealarm getdefalarm
%type <t> maketarget getdeftarget
%type <i> boolean

%%

config:	/* */
	| DEBUG boolean { cur_config.debug=$2; }
	| USER string { cur_config.user=$2; }
	| GROUP string { cur_config.group=$2; }
	| MAILER string { cur_config.group=$2; }
	| TIMESTAMP_FORMAT string { cur_config.timestamp_format=$2; }
	| PID_FILE string { cur_config.pid_file=$2; }
	| STATUS '{' statuscfg '}'
	| RRD INTERVAL TIME { cur_config.rrd_interval=$3; }
	| alarm 
	| target 
	| config separator config
	| error
		{
			logit("Configuration file syntax error. Line %i, character %i",
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
	| MAILSUBJECT string 
		{ cur_alarm->mailsubject=$2; }
	| COMMAND string 
		{ 
			if (cur_alarm->command_on==NULL) cur_alarm->command_on=$2; 
			if (cur_alarm->command_off==NULL) cur_alarm->command_off=$2; 
		}
	| COMMAND ON string 
		{ cur_alarm->command_on=$3; }
	| COMMAND OFF string 
		{ cur_alarm->command_off=$3; }
	| PIPE string 
		{ 
			if (cur_alarm->pipe_on==NULL) cur_alarm->pipe_on=$2; 
			if (cur_alarm->pipe_off==NULL) cur_alarm->pipe_off=$2; 
		}
	| PIPE ON string 
		{ cur_alarm->pipe_on=$3; }
	| PIPE OFF string 
		{ cur_alarm->pipe_off=$3; }
	| COMBINE TIME
		{ cur_alarm->combine_interval=$2; }
	| REPEAT TIME INTEGER
		{ cur_alarm->repeat_interval=$2; cur_alarm->repeat_max=$3; }
	| REPEAT TIME
		{ cur_alarm->combine_interval=$2; cur_alarm->repeat_max=0;}
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
	| RRD FILE_ STRING
		{ cur_target->rrd_filename=$3; }
	| targetcfg separator targetcfg
;

alarmlist: string
		{ $$=alarm2list($1,NULL); }
	| alarmlist ',' string 
		{ $$=alarm2list($3,$1); }
;

statuscfg: /* */ 
	| FILE_ string 
		{ cur_config.status_file=$2; }
	| INTERVAL INTEGER
		{ cur_config.status_interval=$2; }
	| INTERVAL TIME
		{ cur_config.status_interval=$2; }
	| statuscfg separator statuscfg
;


string: STRING	{ $$=pool_strdup(&cur_config.pool,$1); }
;

boolean: ON { $$=1; }
	| OFF { $$=0; }
	| YES { $$=1; }
	| NO { $$=0; }
	| TRUE { $$=1; }
	| FALSE { $$=0; }
;

separator: '\n'
	| ';'
;


%%
void yyerror (const char *s) {
	logit("%s", s);
}

