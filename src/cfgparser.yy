
%{
#include <math.h>
#include "cfgparser.tab.h"
%}

DIGIT	[0-9]

%%
{DIGIT}+ 		{ yylval.i=atoi(yytext); return INTEGER; }

{DIGIT}+("."{DIGIT}+)?(([um]?s)|m|h) { 
			double f;
			double mn=1;
			
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

debug		{ return DEBUG; }
alarm		{ return ALARM; }
target		{ return TARGET; }
default		{ return DEFAULT; }
mailto		{ return MAILTO; }
mailfrom	{ return MAILFROM; }
mailenvfrom	{ return MAILENVFROM; }
down		{ return DOWN; }
loss		{ return LOSS; }
delay		{ return DELAY; }
samples		{ return SAMPLES; }
percent_low	{ return PERCENT_LOW; }
percent_high	{ return PERCENT_HIGH; }
delay_low	{ return DELAY_LOW; }
delay_high	{ return DELAY_HIGH; }
description	{ return DESCRIPTION; }
alarms		{ return ALARMS; }
interval	{ return INTERVAL; }

on|true|yes	{ yylval.i=1; return BOOLEAN; }
off|false|no	{ yylval.i=0; return BOOLEAN; }

\"[^"]*\"		{ yytext[yyleng-1]='\000'; yylval.s=yytext+1; return STRING; }

[{}\n;,]	{ return yytext[0]; }

"//"[^\n]*	/* eat up one-line comments */
"#"[^\n]*	/* eat up one-line comments */

[ \t]+		/* eat up whitespace */

.           printf( "Unrecognized character: %s\n", yytext );

%%

void *p=yyunput;
/* 
 vi: ft=lex 
*/
