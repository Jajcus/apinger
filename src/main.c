#include "config.h"
#include "apinger.h"

#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#ifdef HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef HAVE_GRP_H
# include <grp.h>
#endif

#include "conf.h"
#include "debug.h"

/* Global variables */
struct target *targets=NULL;
struct config default_config={
	NULL,NULL,NULL,		/* pool, alarms, targets */
		{ 		/* alarm defaults */
				AL_NONE,	/* type */
				"default",	/* name */
				"root",		/* mailto */
				"nobody",	/* mailfrom */
				NULL,		/* mailenvfrom */
				{},		/* params */
				NULL		/* next */
		},
		{		/* target defaults */
				"default",	/* name */
				"",		/* description */
				1000,		/* interval */
				20,		/* avg_delay_samples */
				5,		/* avg_loss_delay_samples */
				50,		/* avg_loss_samples */

				NULL,NULL	/* alarms, next */
		},
	0, 			/* debug */
	"nobody",		/* user */
	NULL,			/* group */
	"/var/run/apinger.pid"	/* pid file */
};

int foreground=1;

int icmp_sock;
int icmp6_sock;
int ident;

struct timeval next_probe={0,0};

/* Interrupt handler */
typedef void (*sighandler_t)(int);
volatile int reload_request=0;
volatile int interrupted_by=0;
void signal_handler(int signum){

	if (signum==SIGHUP){
		signal(SIGHUP,signal_handler);
		reload_request=1;
	}
	else{
		interrupted_by=signum;
	}
}

void usage(const char *name){
	fprintf(stderr,"Alarm Pinger 1.0 (c) 2002 Jacek Konieczny <jajcus@pld.org.pl>\n");
	fprintf(stderr,"Usage:\n");
	fprintf(stderr,"\t%s [OPTIONS]\n",name);
	fprintf(stderr,"\nOPTIONS:\n");
	fprintf(stderr,"\t-f\trun in foreground.\n");
	fprintf(stderr,"\t-d\tdebug on.\n");
	fprintf(stderr,"\t-h\tthis message.\n");
}

int main(int argc,char *argv[]){
struct passwd *pw;
struct group *gr;
int c;
FILE *pidfile;
pid_t pid;
int i;
int do_debug=0;
int stay_foreground=0;

	while((c=getopt(argc,argv,"fdh")) != -1){
		switch(c){
			case 'f':
				stay_foreground=1;
				break;
			case 'd':
				do_debug=1;
				break;
			case 'h':
				usage(argv[0]);
				return 1;
			case '?':
				fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				return 1;
			default:
				return 1;
		}
	}
	if (load_config(CONFIG)){
		logit("Couldn't read config (\"%s\").",CONFIG);
		return 1;
	}
	if (do_debug) config->debug=1;

	if (!stay_foreground){
		pidfile=fopen(config->pid_file,"r");
		if (pidfile){
			fscanf(pidfile,"%d",&pid);
			if (pid>0 && kill(pid,0)==0){
				fprintf(stderr,"pinger already running\n");
				return 1;
			}
			fclose(pidfile);
		}
	}

	make_icmp_socket();
	make_icmp6_socket();
	if (icmp6_sock<0 && icmp_sock<0){
		return 1;
	}

	pw=getpwnam(config->user);
	if (!pw) {
		debug("getpwnam(\"%s\") failed.",config->user);
		return 1;
	}
	if (config->group){
		gr=getgrnam(config->group);
		if (!gr) {
			debug("getpwnam(\"%s\") failed.",config->group);
			return 1;
		}
	}
	else gr=NULL;


	if (!stay_foreground){
		pid=fork();
		if (pid<0){
			perror("fork");
			fclose(pidfile);
			exit(1);
		}
		if (pid>0){ /* parent */
			pidfile=fopen(config->pid_file,"w");
			if (!pidfile){
				fprintf(stderr,"Couldn't open pid file for writting.");
				perror(config->pid_file);
				return 1;
			}
			fprintf(pidfile,"%i\n",pid);
			fchown(fileno(pidfile),pw->pw_uid,gr?gr->gr_gid:pw->pw_gid);
			fclose(pidfile);
			exit(0);
		}
		foreground=0;
		for(i=0;i<255;i++)
			if (i!=icmp_sock && i!=icmp6_sock)
				close(i);
		setsid();	
	}
	
	if (initgroups("pinger",pw->pw_gid)){
		myperror("initgroups");
		return 1;
	}
	if (setgid(pw->pw_gid)){
		myperror("setgid");
		return 1;
	}
	if (setuid(pw->pw_uid)){
		myperror("setuid");
		return 1;
	}

	ident=getpid();
	signal(SIGTERM,signal_handler);
	signal(SIGINT,signal_handler);
	signal(SIGHUP,signal_handler);
	signal(SIGPIPE,SIG_IGN);
	main_loop();
	if (icmp_sock>=0) close(icmp_sock);
	if (icmp6_sock>=0) close(icmp6_sock);

	logit("Exiting on signal %i.",interrupted_by);

	if (!foreground){
		/*clear the pid file*/
		pidfile=fopen(config->pid_file,"w");
		if (pidfile) fclose(pidfile);
		/* try to remove it. Most probably this will fail */
		unlink(config->pid_file);
	}

	return 0;
}

