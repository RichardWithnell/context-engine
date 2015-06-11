#ifndef CONTEXT_ACTION
#define CONTEXT_ACTION

#include "debug.h"

enum {
    ACTION_UNSPEC = 0x00,
    ACTION_ENABLE = 0x01,
    ACTION_DISABLE = 0x02,
    ACTION_ADD = 0x03,
    ACTION_REMOVE = 0x04,
    __ACTION_MAX = 0x05,
};

enum {
    ACTION_MODE_UNSPEC = 0x00,
    ACTION_MODE_SOFT = 0x01,
    ACTION_MODE_HARD = 0x02,
    __ACTION_MODE_MAX = 0x03,
};


struct action;

/*Functions*/
int find_action_id(char *action);
void action_print(struct action *a);

/*Memory*/
struct action * action_alloc(void);
void action_free(struct action *as);

/*Getters*/
uint8_t action_get_mode(struct action *a);
uint32_t action_get_action(struct action *a);
char * action_get_link_name(struct action *a);

/*Setters*/
void action_set_action(struct action *a, uint32_t code);
void action_set_link_name(struct action *a, char *link_name);
void action_set_mode(struct action *a, uint8_t mode);

#endif
