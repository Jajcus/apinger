#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <linux/types.h>
#include <linux/filter.h>
#include <sys/time.h>
#include "apinger.h"

/* filter instalation code borrowed from iputils */
void install_filter(){
        static struct sock_filter insns[] = {
                BPF_STMT(BPF_LDX|BPF_B|BPF_MSH, 0), /* Skip IP header. F..g BSD... Look into ping.  */
                BPF_STMT(BPF_LD|BPF_H|BPF_IND, 4), /* Load icmp echo ident */
                BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, 0xAAAA, 0, 1), /* Ours? */
                BPF_STMT(BPF_RET|BPF_K, ~0U), /* Yes, it passes. */
                BPF_STMT(BPF_LD|BPF_B|BPF_IND, 0), /* Load icmp type */
                BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, ICMP_ECHOREPLY, 1, 0), /* Echo? */
                BPF_STMT(BPF_RET|BPF_K, 0xFFFFFFF), /* No. It passes. */
                BPF_STMT(BPF_RET|BPF_K, 0) /* Echo with wrong ident. Reject. */
        };
        static struct sock_fprog filter = {
                sizeof insns / sizeof(insns[0]),
                insns
        };

        /* Patch bpflet for current identifier. */
        insns[2] = (struct sock_filter)BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, htons(ident), 0, 1);

        if (setsockopt(icmp_sock, SOL_SOCKET, SO_ATTACH_FILTER, &filter, sizeof(filter)))
                myperror("WARNING: failed to install socket filter\n");
}

/* function borrowed from iputils */
u_short in_cksum(const u_short *addr, register int len, u_short csum){

	register int nleft = len;
        const u_short *w = addr;
        register u_short answer;
        register int sum = csum;

        /*
         *  Our algorithm is simple, using a 32 bit accumulator (sum),
         *  we add sequential 16 bit words to it, and at the end, fold
         *  back all the carry bits from the top 16 bits into the lower
         *  16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1)
                sum += htons(*(u_char *)w << 8);

        /*
         * add back carry outs from top 16 bits to low 16 bits
         */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return (answer);
}

void send_icmp_probe(struct target *t,struct timeval *cur_time){
static char buf[1024];
struct icmphdr *p=(struct icmphdr *)buf;
struct trace_info ti;
int size;
int ret;
int seq;

	seq=++t->last_sent;

	p->type=ICMP_ECHO;
	p->code=0;
	p->checksum=0;
	p->un.echo.sequence=seq%65536;
	p->un.echo.id=ident;

	ti.timestamp=*cur_time;
	ti.target_id=t;
	ti.seq=seq;
	memcpy(p+1,&ti,sizeof(ti));
	size=sizeof(*p)+sizeof(ti);

	p->checksum = in_cksum((u_short *)p,size,0);
	ret=sendto(icmp_sock,p,size,MSG_DONTWAIT,
			(struct sockaddr *)&t->addr.addr4,sizeof(t->addr.addr4));
	if (ret<0){
		myperror("sendto");
	}
}

void recv_icmp(void){
int len,hlen,icmplen,datalen;
char buf[1024];
struct sockaddr_in from;
struct icmphdr *icmp;
struct iphdr *ip;
struct timeval time_recv;
socklen_t sl;

	sl=sizeof(from);
	gettimeofday(&time_recv,NULL);
	len=recvfrom(icmp_sock,buf,1024,MSG_DONTWAIT,(struct sockaddr *)&from,&sl);
	if (len<0){
		if (errno==EAGAIN) return;
		myperror("recvfrom");
		return;
	}
	if (len==0) return;
	ip=(struct iphdr *)buf;
	hlen=ip->ihl*4;
	if (len<hlen+8 || ip->ihl<5) {
		debug("Too short packet reveiced");
		return;
	}
	icmplen=len-hlen;
	icmp=(struct icmphdr *)(buf+hlen);
	if (icmp->type != ICMP_ECHOREPLY){
		debug("Other (%i) icmp type received",icmp->type);
		return;
	}
	if (icmp->un.echo.id != ident){
		debug("Alien echo-reply received");
		return;
	}
	debug("Ping reply from %s",inet_ntoa(from.sin_addr));
	datalen=icmplen-sizeof(*icmp);
	if (datalen!=sizeof(struct trace_info)){
		debug("Packet data truncated.");
		return;
	}
	analyze_reply(time_recv,icmp->un.echo.sequence,(struct trace_info*)(icmp+1));
}

int make_icmp_socket(void){

	icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (icmp_sock<0)
		myperror("socket");
	/*else
		install_filter();*/
	return icmp_sock;
}

