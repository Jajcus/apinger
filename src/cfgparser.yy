
%{
#include <math.h>
#include "cfgparser.tab.h"

#define LOC yylloc.first_line=yylloc.last_line; yylloc.first_column=yylloc.last_column
#define LOCINC yylloc.last_column+=yyleng

extern YYLTYPE yylloc;
%}

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

debug		{ LOC; LOCINC; return DEBUG; }
alarm		{ LOC; LOCINC; return ALARM; }
target		{ LOC; LOCINC; return TARGET; }
default		{ LOC; LOCINC; return DEFAULT; }
mailto		{ LOC; LOCINC; return MAILTO; }
mailfrom	{ LOC; LOCINC; return MAILFROM; }
mailenvfrom	{ LOC; LOCINC; return MAILENVFROM; }
down		{ LOC; LOCINC; return DOWN; }
loss		{ LOC; LOCINC; return LOSS; }
delay		{ LOC; LOCINC; return DELAY; }
samples		{ LOC; LOCINC; return SAMPLES; }
percent_low	{ LOC; LOCINC; return PERCENT_LOW; }
percent_high	{ LOC; LOCINC; return PERCENT_HIGH; }
delay_low	{ LOC; LOCINC; return DELAY_LOW; }
delay_high	{ LOC; LOCINC; return DELAY_HIGH; }
description	{ LOC; LOCINC; return DESCRIPTION; }
alarms		{ LOC; LOCINC; return ALARMS; }
interval	{ LOC; LOCINC; return INTERVAL; }

on|true|yes	{ LOC; LOCINC; yylval.i=1; return BOOLEAN; }
off|false|no	{ LOC; LOCINC; yylval.i=0; return BOOLEAN; }

\"[^"\n]*\"	{ LOC; LOCINC; yytext[yyleng-1]='\000'; yylval.s=yytext+1; return STRING; }

[{};,]		{ LOC; LOCINC; return yytext[0]; }
\n		{ LOC; yylloc.last_line++; yylloc.last_column=0; return '\n'; }

"//"[^\n]*	{ LOC; LOCINC; } 
"#"[^\n]*	{ LOC; LOCINC; }

[ \t]+		{ LOC; LOCINC; }

.           	{ LOC; LOCINC; yylval.s=yytext; return ERROR; }

%%

void *p=yyunput;
/* 
 vi: ft=lex 
*/
