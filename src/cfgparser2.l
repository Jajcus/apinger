
%{
#include <math.h>
#include "cfgparser.tab.h"
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
avg_loss_samples	{ LOC; LOCINC; return AVG_LOSS_SAMPLES; }
avg_loss_delay_samples	{ LOC; LOCINC; return AVG_LOSS_DELAY_SAMPLES; }
debug		{ LOC; LOCINC; return DEBUG; }
default		{ LOC; LOCINC; return DEFAULT; }
delay		{ LOC; LOCINC; return DELAY; }
delay_high	{ LOC; LOCINC; return DELAY_HIGH; }
delay_low	{ LOC; LOCINC; return DELAY_LOW; }
description	{ LOC; LOCINC; return DESCRIPTION; }
down		{ LOC; LOCINC; return DOWN; }
group		{ LOC; LOCINC; return GROUP; }
interval	{ LOC; LOCINC; return INTERVAL; }
loss		{ LOC; LOCINC; return LOSS; }
mailenvfrom	{ LOC; LOCINC; return MAILENVFROM; }
mailfrom	{ LOC; LOCINC; return MAILFROM; }
mailto		{ LOC; LOCINC; return MAILTO; }
percent_high	{ LOC; LOCINC; return PERCENT_HIGH; }
percent_low	{ LOC; LOCINC; return PERCENT_LOW; }
target		{ LOC; LOCINC; return TARGET; }
time		{ LOC; LOCINC; return TIME_; }
user		{ LOC; LOCINC; return USER; }

on|true|yes	{ LOC; LOCINC; yylval.i=1; return BOOLEAN; }
off|false|no	{ LOC; LOCINC; yylval.i=0; return BOOLEAN; }

\"[^"\n]*\"	{ LOC; LOCINC; yytext[yyleng-1]='\000'; yylval.s=yytext+1; return STRING; }

[{};,]		{ LOC; LOCINC; return yytext[0]; }
\n		{ LOC; yylloc.last_line++; yylloc.last_column=0; return '\n'; }

"//"[^\n]*	{ LOC; LOCINC; } 
"#"[^\n]*	{ LOC; LOCINC; }

[ \t]+		{ LOC; LOCINC; }

.           	{ LOC; LOCINC; yylval.s=yytext; 
		  log("Configuration file syntax error. Line %i, character %i ('%c')",
					yylloc.first_line+1,yylloc.first_column+1,yytext[0]);
		  return ERROR;
		}

%%

void *p=yyunput;
/* 
 vi: ft=lex 
*/
