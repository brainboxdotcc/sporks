/************************************************************************************
 * 
 * Sporks, the learning, scriptable Discord bot!
 *
 * Copyright 2019 Craig Edwards <support@sporks.gg>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ************************************************************************************/

#include "duktape.h"
#include <sys/time.h>

int interrupt = 0;
struct timeval t_script_start;
struct timeval t_script_now;

duk_bool_t check_exec_timeout(void *udata)
{
	gettimeofday(&t_script_now, NULL);
	uint64_t microsecs = (t_script_now.tv_sec - t_script_start.tv_sec) * 1000000 + t_script_now.tv_usec - t_script_start.tv_usec;

        return (microsecs > 10000 ? 1 : 0);
}

