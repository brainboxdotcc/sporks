#include "duktape.h"

int interrupt = 0;

duk_bool_t check_exec_timeout(void *udata)
{
        return interrupt;
}

