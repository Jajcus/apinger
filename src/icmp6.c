#include "config.h"
#ifdef HAVE_IPV6

#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_NETINET_ICMP6_H
# include <netinet/icmp6.h>
#endif
#ifdef HAVE_NETINET_IP6_H
# include <netinet/ip6.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#include "apinger.h"
#include "debug.h"

#ifdef HAVE_LINUX_FILTER_H
# ifdef HAVE_LINUX_TYPES_H
#  include <linux/types.h>
# endif
# include <linux/filter.h>
#endif /* HAVE_LINUX_FILTER_H */

void install_filter6(){

#ifdef HAVE_LINUX_FILTER_H
        static struct sock_filter insns[] = {
                BPF_STMT(BPF_LD|BPF_H|BPF_ABS, 4),  /* Load icmp echo ident */
                BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0xAAAA, 0, 1),  /* Ours? */
                BPF_STMT(BPF_RET|BPF_K, ~0U),  /* Yes, it passes. */
                BPF_STMT(BPF_LD|BPF_B|BPF_ABS, 0),  /* Load icmp type */
                BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, ICMP6_ECHO_REPLY, 1, 0), /* Echo? */
                BPF_STMT(BPF_RET|BPF_K, ~0U), /* No. It passes. This must not happen. */
                BPF_STMT(BPF_RET|BPF_K, 0), /* Echo with wrong ident. Reject. */
        };
        static struct sock_fprog filter = {
                sizeof insns / sizeof(insns[0]),
                insns
        };

        insns[1] = (struct sock_filter)BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, htons(ident), 0, 1);

        if (setsockopt(icmp6_sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)))
                myperror("WARNING: failed to install socket filter\n");
#endif /* HAVE_LINUX_FILTER_H */

}


void send_icmp6_probe(struct target *t,struct timeval *cur_time,int seq){
static char buf[1024];
struct icmp6_hdr *p=(struct icmp6_hdr *)buf;
struct trace_info ti;
int size;
int ret;

	p->icmp6_type=ICMP6_ECHO_REQUEST;
	p->icmp6_code=0;
	p->icmp6_cksum=0;
	p->icmp6_seq=seq%65536;
	p->icmp6_id=ident;

	ti.timestamp=*cur_time;
	ti.target_id=t;
	ti.seq=seq;
	memcpy(p+1,&ti,sizeof(ti));
	size=sizeof(*p)+sizeof(ti);

	ret=sendto(icmp6_sock,p,size,MSG_DONTWAIT,
			(struct sockaddr *)&t->addr.addr6,sizeof(t->addr.addr6));
	if (ret<0){
		if (config->debug) myperror("sendto");
	}
}

void recv_icmp6(void){
int len,icmplen,datalen;
char buf[1024];
char abuf[100];
const char *name;
struct sockaddr_in6 from;
struct icmp6_hdr *icmp;
socklen_t sl;
struct timeval time_recv;

	sl=sizeof(from);
	gettimeofday(&time_recv,NULL);
	len=recvfrom(icmp6_sock,buf,1024,0,(struct sockaddr *)&from,&sl);
	if (len<0){
		if (errno==EAGAIN) return;
		myperror("recvfrom");
		return;
	}
	if (len==0) return;

	icmplen=len;
	icmp=(struct icmp6_hdr *)buf;
	if (icmp->icmp6_type != ICMP6_ECHO_REPLY) return;
	if (icmp->icmp6_id != ident) return;

	name=inet_ntop(AF_INET6,&from.sin6_addr,abuf,100);
	debug("Ping reply from %s",name);
	datalen=icmplen-sizeof(*icmp);
	if (datalen!=sizeof(struct trace_info)){
		debug("Packet data truncated.");
		return;
	}
	analyze_reply(time_recv,icmp->icmp6_seq,(struct trace_info*)(icmp+1));
}


int make_icmp6_socket(void){
int opt;

	icmp6_sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
	if (icmp6_sock<0)
		myperror("socket");
	else {
		opt=2;
		if (setsockopt(icmp6_sock, SOL_RAW, IPV6_CHECKSUM, &opt, sizeof(int)))
			myperror("setsockopt(IPV6_CHECKSUM)");
		/*install_filter6();*/
	}
	return icmp6_sock;
}

#else /*HAVE_IPV6*/
#include "apinger.h"

int make_icmp6_socket(void){ return -1; }
void recv_icmp6(void){}
void send_icmp6_probe(struct target *t,struct timeval *cur_time,int seq){}

#endif /*HAVE_IPV6*/
