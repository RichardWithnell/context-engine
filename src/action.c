#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "action.h"

#define ACTION_LINK_NAME_SIZE 32

#define ACTIONKEY_TO_STR(x) action_keystr[x]

static const char *action_keystr[__ACTION_MAX] = {
    [ACTION_UNSPEC] = "unspec",
    [ACTION_ENABLE] = "enable",
    [ACTION_DISABLE] = "disable",
    [ACTION_ADD] = "add",
    [ACTION_REMOVE] = "remove",
};

struct action
{
    void *data;
    uint8_t mode;
    uint32_t force;
    uint32_t action;
    char link_name[ACTION_LINK_NAME_SIZE];
};


int find_action_id(char *act)
{
    int i = 0;
    for(; i < __ACTION_MAX; i++){
        if(!strcmp(action_keystr[i], act)){
            return i;
        }
    }
    return -1;
}

void action_print(struct action *a)
{
    printf("Action: %s %s\n", ACTIONKEY_TO_STR(a->action), a->link_name);
}

struct action * action_alloc(void)
{
    struct action *a = (struct action*)0;
    a = malloc(sizeof(struct action));
    return a;
}

void action_free(struct action *as)
{
    free(as);
}

uint8_t action_get_mode(struct action *a)
{
    return a->mode;
}

uint32_t action_get_action(struct action *a)
{
    return a->action;
}

char * action_get_link_name(struct action *a)
{
    return a->link_name;
}

void action_set_mode(struct action *a, uint8_t mode)
{
    a->mode = mode;
}

void action_set_action(struct action *a, uint32_t code)
{
    a->action = code;
}

void action_set_link_name(struct action *a, char *link_name)
{
    memset(a->link_name, 0, ACTION_LINK_NAME_SIZE);
    strncpy(a->link_name, link_name, strlen(link_name));
}
