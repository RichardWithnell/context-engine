#ifndef CONTEXT_POLICY
#define CONTEXT_POLICY

#include "list.h"
#include "conditions/condition.h"
#include "action.h"
#include "context_library.h"

struct context_library;

typedef void (*policy_cb_t)(struct condition *c, void *data);

List * load_policy_file(char *config_file, List * context_libs);
int load_condition_lib(struct context_library *ch, List * context_libs);

#endif
