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
	char *pid_file;
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
