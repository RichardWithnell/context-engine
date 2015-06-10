#ifndef CONTEXT_ACTION
#define CONTEXT_ACTION

#include "policy.h"
#include "debug.h"

enum {
    ACTION_UNSPEC = 0x00,
    ACTION_ENABLE = 0x01,
    ACTION_DISABLE = 0x02,
    ACTION_ADD = 0x03,
    ACTION_REMOVE = 0x04,
    __ACTION_MAX = 0x05,
};

struct action;

int find_action_id(char *action);

void action_print(struct action *a);
struct action * action_alloc(void);

void action_set_action(struct action *a, uint32_t code);
void action_set_link_name(struct action *a, char *link_name);

#endif
