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
 *  $Id: apinger.h,v 1.8 2002/07/17 09:32:51 cvs-jajcus Exp $
 */

#ifndef apinger_h
#define apinger_h

#define CONFIG SYSCONFDIR "/apinger.conf"

#if TIME_WITH_SYS_TIME
# include <time.h>
# include <sys/time.h>
#else
#  ifdef HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
# include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#include "conf.h"

union addr {
	struct sockaddr addr;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;
};

struct target {
	char *name;		/* name (IP address as string) */
	
	struct timeval interval_tv;
	union addr addr;	/* target address */
	
	char *queue;		/*
				contains info about recently sent packets
				"1" means it was received */
	int last_sent;		/* sequence number of the last ping sent */
	int last_received;	/* sequence number of the last ping received */
	struct timeval last_received_tv; /* timestamp of the last ping received */
	int received;		/* number of packets received */
	int recently_lost;	/* number of packets lost between
				   last_sent-200 to last_sent-100
				   for avg. lost computation */
	double *rbuf;		/* bufor of received pings
				   (for avarage delay computation) */
	double delay_sum;

	struct timeval next_probe; /* time when next probe is scheduled */

	struct alarm_list *active_alarms;
	struct target_cfg *config;
	
	struct target *next;
};

struct trace_info {
	struct timeval timestamp;
	int seq;
	void *target_id;
};

struct target *targets;

extern int foreground;

extern int icmp_sock;
extern int icmp6_sock;
extern int ident;

extern struct timeval next_probe;

int make_icmp_socket(void);
void recv_icmp(void);
void send_icmp_probe(struct target *t,struct timeval *cur_time,int seq);

int make_icmp6_socket(void);
void recv_icmp6(void);
void send_icmp6_probe(struct target *t,struct timeval *cur_time,int seq);

void analyze_reply(struct timeval time_recv,int seq,struct trace_info *ti);
void main_loop(void);

extern volatile int interrupted_by;
extern volatile int reload_request;

#endif
