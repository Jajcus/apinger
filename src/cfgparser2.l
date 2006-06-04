/*
 *  Alarm Pinger (c) 2002 Jacek Konieczny <jajcus@jajcus.net>
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
 *  $Id$
 */


%{
#include <math.h>
#include "cfgparser1.h"
#include "debug.h"

#define LOC yylloc.first_line=yylloc.last_line; yylloc.first_column=yylloc.last_column
#define LOCINC yylloc.last_column+=yyleng
extern YYLTYPE yylloc;

%}
%option noyywrap


DIGIT	[0-9]

%%

{DIGIT}+ 		{ LOC; LOCINC; yylval.i=atoi(yytext); return INTEGER; }

{DIGIT}+("."{DIGIT}+)?(([um]?s)|m|h) { 
			double f;
			double mn=1;

			LOC;
			LOCINC; 
			
			if (yyleng>1 && yytext[yyleng-1]=='h'){
				yytext[yyleng-1]='\000';
				mn=60*60*1000;
			}
			else if (yyleng>1 && yytext[yyleng-1]=='m'){
				yytext[yyleng-1]='\000';
				mn=60*1000;
			}
			else if (yyleng>1 && yytext[yyleng-1]=='s'){
				yytext[yyleng-1]='\000';
				mn=1000;
				if (yyleng>2 && yytext[yyleng-2]=='m'){
					yytext[yyleng-1]='\000';
					mn=1;
				}
				else if (yyleng>2 && yytext[yyleng-2]=='u'){
					yytext[yyleng-1]='\000';
					mn=0.001;
				}
			}
			f=atof(yytext)*mn;
			yylval.i=f; return TIME;
		}

alarm		{ LOC; LOCINC; return ALARM; }
alarms		{ LOC; LOCINC; return ALARMS; }
avg_delay_samples	{ LOC; LOCINC; return AVG_DELAY_SAMPLES; }
avg_loss_delay_samples	{ LOC; LOCINC; return AVG_LOSS_DELAY_SAMPLES; }
avg_loss_samples	{ LOC; LOCINC; return AVG_LOSS_SAMPLES; }
combine		{ LOC; LOCINC; return COMBINE; }
command		{ LOC; LOCINC; return COMMAND; }
debug		{ LOC; LOCINC; return DEBUG; }
default		{ LOC; LOCINC; return DEFAULT; }
delay		{ LOC; LOCINC; return DELAY; }
delay_high	{ LOC; LOCINC; return DELAY_HIGH; }
delay_low	{ LOC; LOCINC; return DELAY_LOW; }
description	{ LOC; LOCINC; return DESCRIPTION; }
down		{ LOC; LOCINC; return DOWN; }
false		{ LOC; LOCINC; return FALSE; }
file		{ LOC; LOCINC; return FILE_; }
group		{ LOC; LOCINC; return GROUP; }
interval	{ LOC; LOCINC; return INTERVAL; }
loss		{ LOC; LOCINC; return LOSS; }
mailenvfrom	{ LOC; LOCINC; return MAILENVFROM; }
mailer		{ LOC; LOCINC; return MAILER; }
mailfrom	{ LOC; LOCINC; return MAILFROM; }
mailsubject	{ LOC; LOCINC; return MAILSUBJECT; }
mailto		{ LOC; LOCINC; return MAILTO; }
no		{ LOC; LOCINC; return NO; }
off		{ LOC; LOCINC; return OFF; }
on		{ LOC; LOCINC; return ON; }
override	{ LOC; LOCINC; return OVERRIDE; }
percent_high	{ LOC; LOCINC; return PERCENT_HIGH; }
percent_low	{ LOC; LOCINC; return PERCENT_LOW; }
pid_file	{ LOC; LOCINC; return PID_FILE; }
pipe		{ LOC; LOCINC; return PIPE; }
repeat		{ LOC; LOCINC; return REPEAT; }
rrd		{ LOC; LOCINC; return RRD; }
status		{ LOC; LOCINC; return STATUS; }
target		{ LOC; LOCINC; return TARGET; }
time		{ LOC; LOCINC; return TIME_; }
timestamp_format { LOC; LOCINC; return TIMESTAMP_FORMAT; }
true		{ LOC; LOCINC; return TRUE; }
user		{ LOC; LOCINC; return USER; }
yes		{ LOC; LOCINC; return YES; }

\"[^"\n]*\"	{ LOC; LOCINC; yytext[yyleng-1]='\000'; yylval.s=yytext+1; return STRING; }

[{};,]		{ LOC; LOCINC; return yytext[0]; }
\n		{ LOC; yylloc.last_line++; yylloc.last_column=0; return '\n'; }

"//"[^\n]*	{ LOC; LOCINC; } 
"#"[^\n]*	{ LOC; LOCINC; }

[ \t]+		{ LOC; LOCINC; }

.           	{ LOC; LOCINC; yylval.s=yytext; 
		  logit("Unexpected character: '%c'", yytext[0]);
		  return ERROR;
		}

%%

void *p=yyunput;
/* 
 vi: ft=lex 
*/
