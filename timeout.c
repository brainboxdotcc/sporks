#include "duktape.h"
#include <sys/time.h>

int interrupt = 0;
struct timeval t_script_start;

duk_bool_t check_exec_timeout(void *udata)
{
	struct timeval t_script_now;
	gettimeofday(&t_script_now, NULL);
	uint64_t microsecs = (t_script_now.tv_sec - t_script_start.tv_sec) * 1000000 + t_script_now.tv_usec - t_script_start.tv_usec;

        return (microsecs > 10000 ? 1 : 0);
}

