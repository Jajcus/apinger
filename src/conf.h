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
 *  $Id: conf.h,v 1.6 2002/07/18 09:33:06 cvs-jajcus Exp $
 */

#ifndef conf_h
#define conf_h

struct pool_item {
	struct pool_item *next;
};

void *pool_malloc(struct pool_item **pool,size_t size);
char *pool_strdup(struct pool_item **pool,const char *str);
void pool_free(struct pool_item **pool,void *ptr);
void pool_clear(struct pool_item **pool);

enum alarm_type {
	AL_NONE=-1,
	AL_DOWN=0,
	AL_DELAY,
	AL_LOSS,
	NR_ALARMS
};

struct alarm_cfg {
	enum alarm_type type;
	char *name;
	char *mailto;
	char *mailfrom;
	char *mailenvfrom;
	char *command_on;
	char *command_off;
	char *pipe;
	union {
		int val;
		struct {
			int low;
			int high;
		}lh;
	}p;
	struct alarm_cfg *next;
};

struct alarm_list {
	struct alarm_cfg *alarm;
	struct alarm_list *next;
};

struct target_cfg {
	char *name;
	char *description;
	int interval;
	int avg_delay_samples;
	int avg_loss_delay_samples;
	int avg_loss_samples;
	struct alarm_list *alarms;
	
	struct target_cfg *next;
};

struct config {
	struct pool_item *pool;
	struct alarm_cfg *alarms;
	struct target_cfg *targets;
	struct alarm_cfg alarm_defaults;
	struct target_cfg target_defaults;
	int debug;
	char *user;
	char *group;
	char *mailer;
	char *pid_file;
	char *status_file;
	int status_interval;
};

extern struct config cur_config,default_config;
extern struct config * config;
extern struct alarm_cfg *cur_alarm;
extern struct target_cfg *cur_target;

struct alarm_cfg * make_alarm();
struct target_cfg * make_target();
void add_alarm(enum alarm_type type);
void add_target(void);
struct alarm_list *alarm2list(const char *aname,struct alarm_list *list);

int load_config(const char *filename);

#endif
