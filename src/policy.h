#ifndef CONTEXT_POLICY
#define CONTEXT_POLICY

#include "list.h"
#include "conditions/condition.h"
#include "action.h"
#include "context_library.h"

struct policy_definition;

typedef void (*policy_cb_t)(struct condition *c, void *data);

typedef void (*policy_list_condition_cb_t)(struct condition *c, void *data);
typedef void (*policy_list_action_cb_t)(struct action *a, void *data);


List * load_policy_file(char *config_file, List * context_libs);
int load_condition_lib(struct context_library *ch, List * context_libs);


void policy_definition_for_each_action(struct policy_definition *pd, policy_list_action_cb_t cb, void *data);
void policy_definition_for_each_condition(struct policy_definition *pd, policy_list_condition_cb_t cb, void *data);
struct policy_definition * policy_definition_alloc();
void policy_definition_free(struct policy_definition *pd);
List *policy_definition_get_conditions(struct policy_definition *pd);
List *policy_definition_get_actions(struct policy_definition *pd);
void policy_definition_put_action(struct policy_definition *pd, struct action *act);
void policy_definition_put_condition(struct policy_definition *pd, struct condition *condition);
void policy_defintiion_set_conditions_met(struct policy_definition *pd, uint8_t conditions_met);
void policy_definition_set_actions_performed(struct policy_definition *pd, uint8_t actions_performed);

#endif
