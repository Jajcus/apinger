#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include "apinger.h"

/* Global variables */
struct target_config current_config={1000,-1,-1,-1,-1,-1,NULL,NULL};
struct target *targets=NULL;

int cf_debug=0;
int foreground=0;

int icmp_sock;
int icmp6_sock;
int ident;

struct timeval next_probe={0,0};

/* Interrupt handler */
typedef void (*sighandler_t)(int);
volatile int signal_received=0;
void signal_handler(int signum){

	signal_received=signum;
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
int c;
FILE *pidfile;
pid_t pid;
int i;

	while((c=getopt(argc,argv,"fdh")) != -1){
		switch(c){
			case 'f':
				foreground=1;
				break;
			case 'd':
				cf_debug=1;
				break;
			case 'h':
				usage(argv[0]);
				return 1;
			case '?':
				if (isprint (optopt))
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				else
				        fprintf (stderr, "Unknown option character `\\x%x'.\n",
							optopt);
				return 1;
			default:
				return 1;
		}
	}
	if (!foreground){
		pidfile=fopen(PID_FILE,"r");
		if (pidfile){
			fscanf(pidfile,"%d",&pid);
			if (pid>0 && kill(pid,0)==0){
				fprintf(stderr,"pinger already running\n");
				return 1;
			}
			fclose(pidfile);
		}
		pidfile=fopen(PID_FILE,"w");
		if (!pidfile){
			fprintf(stderr,"Couldn't open pid file.");
			perror(PID_FILE);
			return 1;
		}
	}

	make_icmp_socket();
	make_icmp6_socket();
	if (icmp6_sock<0 && icmp_sock<0){
		return 1;
	}

	pw=getpwnam("pinger");
	if (!pw) {
		debug("getpwnam(\"pinger\") failed.");
		return 1;
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

	read_config();

	if (!foreground){
		pid=fork();
		if (pid<0){
			perror("fork");
			fclose(pidfile);
			exit(1);
		}
		if (pid>0){ /* parent */
			fprintf(pidfile,"%i\n",pid);
			fclose(pidfile);
			exit(0);
		}
		for(i=0;i<255;i++)
			if (i!=icmp_sock && i!=icmp6_sock)
				close(i);
		setsid();	
	}
	
	ident=getpid();
	signal(SIGTERM,signal_handler);
	signal(SIGINT,signal_handler);
	signal(SIGPIPE,SIG_IGN);
	main_loop();
	if (icmp_sock>=0) close(icmp_sock);
	if (icmp6_sock>=0) close(icmp6_sock);

	log("Exiting on signal %i.",signal_received);

	return 0;
}

