#ifndef conf_h
#define conf_h

struct pool_item {
	void *ptr;
	struct pool_item *next;
};

struct default_cfg {
	char *mailto;
	char *mailfrom;
	char *mailenvfrom;
	int interval;
};

enum alarm_type {
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
	struct alarm_list *alarms;
	
	struct target_cfg *next;
};

struct config {
	struct pool_item *pool;
	struct alarm_cfg *alarm_defaults;
	struct alarm_cfg *alarms;
	struct target_cfg *target_defaults;
	struct target_cfg *targets;
	int debug;
};

#endif
