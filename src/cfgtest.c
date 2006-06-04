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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
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
