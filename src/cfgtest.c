#include <stdlib.h>
#include "conf.h"

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


int main(int argc,char *argv[]){
int ret;
	
	ret=load_config("/etc/apinger.conf");
	return ret;
}
