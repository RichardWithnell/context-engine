#ifndef CONTEXT_LIBRARY
#define CONTEXT_LIBRARY

#include "conditions/condition.h"
#include "policy.h"
#include "debug.h"

#define CONTEXT_LIBRARY_PATH_SIZE 512

struct context_library;

int load_context_libs(List *list, char *libs[]);

parse_condition_t context_library_get_parser(List *list, char * key);
struct context_library * context_library_get_for_key(List *list, char * key);
int context_library_add_condition(List *context_libs, struct condition *cond);

int context_library_start(List *context_libs, condition_cb_t start);

int context_library_check_loaded(List *list, char *path);
int context_library_load(struct context_library *ch);

struct context_library *context_library_alloc(void);
void context_library_free(struct context_library *ch);

void context_library_set_id(struct context_library *ch, int id);
void context_library_set_path(struct context_library *ch, char* path);
void context_library_set_init(struct context_library *ch, init_condition_t init);

char * context_library_get_path(struct context_library *ch);
init_condition_t context_library_get_init(struct context_library *ch);

#endif
