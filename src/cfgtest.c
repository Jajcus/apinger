#include <unistd.h>
#include "conf.h"

int main(int argc,char *argv[]){
int ret;
	
	ret=load_config("/etc/apinger.conf");
	return ret;
}
