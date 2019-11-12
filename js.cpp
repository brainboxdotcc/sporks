#include "js.h"
#include <thread>
#include <streambuf>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>

extern int interrupt;
bool terminate = false;
int64_t current_context;
static std::shared_ptr<spdlog::logger> c_apis_suck;
std::unordered_map<int64_t, duk_context*> emptyref;
std::unordered_map<int64_t, duk_context*> &contexts = emptyref;
std::unordered_map<int64_t, size_t> total_allocated;

void sandbox_fatal(void *udata, const char *msg);
void sandbox_free(void *udata, void *ptr);
void *sandbox_alloc(void *udata, duk_size_t size);
static void *sandbox_realloc(void *udata, void *ptr, duk_size_t size);

struct program
{
	std::string name;
	std::string source;
};

std::unordered_map<int64_t, program> code;

struct alloc_hdr {
	/* The double value in the union is there to ensure alignment is
	 * good for IEEE doubles too.  In many 32-bit environments 4 bytes
	 * would be sufficiently aligned and the double value is unnecessary.
	 */
	union {
		size_t sz;
		double d;
	} u;
};

static size_t max_allocated = 256 * 1024;  /* 256kB sandbox */

static void define_func(duk_context* ctx, const char *name, duk_c_function func, int nargs)
{
	duk_push_string(ctx, name);
	duk_push_c_function(ctx, func, nargs);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
}

static void define_number(duk_context* ctx, const char *name, duk_double_t num)
{
	duk_push_string(ctx, name);
	duk_push_number(ctx, num);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
}

static void define_string(duk_context* ctx, const char *name, const char *value)
{
	duk_push_string(ctx, name);
	duk_push_string(ctx, value);
	duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_VALUE);
}

static duk_ret_t js_print(duk_context *cx)
{
	int argc = duk_get_top(cx);
	if (argc < 1)
		return 0;
	for (int i = 0; i < argc; i++)
		c_apis_suck->warn("JS debuglog(): {}", duk_to_string(cx, i - argc));
	return 0;
}

JS::JS(std::shared_ptr<spdlog::logger>& logger) : log(logger)
{
}

bool JS::run(int64_t channel_id, const std::unordered_map<std::string, json> &vars)
{
	duk_int_t ret;
	int i;

	auto iter = code.find(channel_id);
	duk_context* ctx;
	program v;

	c_apis_suck = log;

	if (iter == code.end()) {

		log->info("create new context for channel {}", channel_id);
		std::string name = "../js/" + std::to_string(channel_id) + ".js";
		std::ifstream src(name);
		std::string source((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
		v.name = name;
		v.source = source;

		code[channel_id] = v;

	} else {
		v = code[channel_id];
	}

	current_context = channel_id;
	total_allocated[channel_id] = 0;
	ctx = duk_create_heap(sandbox_alloc, sandbox_realloc, sandbox_free, NULL, sandbox_fatal);

	duk_push_global_object(ctx);
	define_number(ctx, "THIS_CHANNEL_ID", (duk_double_t)channel_id);
	define_func(ctx, "debuglog", js_print, DUK_VARARGS);
	duk_pop(ctx);

	duk_push_string(ctx, v.name.c_str());
	std::string source;
	for (auto i = vars.begin(); i != vars.end(); ++i) {
		source += i->first + "=" + i->second.dump() + ";\n";
	}
	source += "\n" + v.source;
	if (duk_pcompile_string_filename(ctx, 0, source.c_str()) != 0) {
		lasterror = duk_safe_to_string(ctx, -1);
		log->error("couldnt compile: {}", lasterror);
		return false;
	}

	if (/*!duk_get_global_string(ctx, "onmessage") ||*/ !duk_is_function(ctx, -1)) {
		lasterror = "No onmessage function found";
		log->error("JS error: {}", lasterror);
		return false;
	}
	do {
		interrupt = 0;
		terminate = false;
		const uint64_t timeout = 10;
		auto monitor = std::thread([]() {
			uint64_t ms = 0;
			for (ms = 0; !terminate && ms < timeout; ++ms) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			if (ms >= timeout) {
				interrupt = 1;
			}
		});
		ret = duk_pcall(ctx, 0);
		terminate = true;
		monitor.join();
	} while (false);

	if (ret != DUK_EXEC_SUCCESS) {
		if (duk_is_error(ctx, -1)) {
			duk_get_prop_string(ctx, -1, "stack");
			lasterror = duk_safe_to_string(ctx, -1);
			duk_pop(ctx);
		} else {
			lasterror = duk_safe_to_string(ctx, -1);
		}
		log->error("JS error: {}", lasterror);
		return false;
	}
	log->error("Executed JS on channel {}", channel_id);
	//function return value
	duk_pop(ctx);
	duk_destroy_heap(ctx);
	return true;
}

void sandbox_fatal(void *udata, const char *msg) {
	// Yeah, according to the docs a fatal can never return. Technically, it doesnt.
	// At this point we should probably destroy the duk context as bad.
	std::string error = msg;
	total_allocated[current_context] = 0;
	throw std::runtime_error("JS error: " + error);
}

void sandbox_free(void *udata, void *ptr) {
	alloc_hdr *hdr;

	if (!ptr) {
		return;
	}
	hdr = (alloc_hdr *) (((char *) ptr) - sizeof(alloc_hdr));
	auto iter = total_allocated.find(current_context);
	iter->second -= hdr->u.sz;
	free((void *) hdr);
}

void *sandbox_alloc(void *udata, duk_size_t size) {
	alloc_hdr *hdr;

	if (size == 0) {
		return NULL;
	}

	auto iter = total_allocated.find(current_context);

	if (iter->second + size > max_allocated) {
		c_apis_suck->error("Sandbox maximum allocation size reached, {} requested in sandbox_alloc", (long) size);
		return NULL;
	}

	hdr = (alloc_hdr *) malloc(size + sizeof(alloc_hdr));
	if (!hdr) {
		return NULL;
	}
	hdr->u.sz = size;
	iter->second += size;
	return (void *) (hdr + 1);
}

static void *sandbox_realloc(void *udata, void *ptr, duk_size_t size) {
	alloc_hdr *hdr;
	size_t old_size;
	void *t;

	auto iter = total_allocated.find(current_context);

	/* Handle the ptr-NULL vs. size-zero cases explicitly to minimize
	 * platform assumptions.  You can get away with much less in specific
	 * well-behaving environments.
	 */

	if (ptr) {
		hdr = (alloc_hdr *) (((char *) ptr) - sizeof(alloc_hdr));
		old_size = hdr->u.sz;

		if (size == 0) {
			iter->second -= old_size;
			free((void *) hdr);
			return NULL;
		} else {
			if (iter->second - old_size + size > max_allocated) {
				c_apis_suck->error("Sandbox maximum allocation size reached, {} requested in sandbox_realloc", (long) size);
				return NULL;
			}

			t = realloc((void *) hdr, size + sizeof(alloc_hdr));
			if (!t) {
				return NULL;
			}
			hdr = (alloc_hdr *) t;
			iter->second -= old_size;
			iter->second += size;
			hdr->u.sz = size;
			return (void *) (hdr + 1);
		}
	} else {
		if (size == 0) {
			return NULL;
		} else {
			if (iter->second + size > max_allocated) {
				c_apis_suck->error("Sandbox maximum allocation size reached, {} requested in sandbox_realloc", (long) size);
				return NULL;
			}

			hdr = (alloc_hdr *) malloc(size + sizeof(alloc_hdr));
			if (!hdr) {
				return NULL;
			}
			hdr->u.sz = size;
			iter->second += size;
			return (void *) (hdr + 1);
		}
	}
}


