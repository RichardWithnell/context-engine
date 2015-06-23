#include "policy.h"


struct policy_definition
{
    uint8_t conditions_met;
    uint8_t actions_performed;
    List *conditions;
    List *actions;
};

void policy_definition_put(List *list, void *data)
{
    Litem *item;
    item = malloc(sizeof(Litem));
    item->data = data;
    list_put(list, item);
}

void policy_definition_for_each_action(struct policy_definition *pd, policy_list_action_cb_t cb, void *data)
{
    Litem *item;

    list_for_each(item, pd->conditions){
        struct action *a;
        a = (struct action *)item->data;
        cb(a, data);
    }
}

void policy_definition_for_each_condition(struct policy_definition *pd, policy_list_condition_cb_t cb, void *data)
{
    Litem *item;

    list_for_each(item, pd->actions){
        struct condition *c;
        c = (struct condition *)item->data;
        cb(c, data);
    }
}

struct policy_definition * policy_definition_alloc()
{
    struct policy_definition *pd;

    pd = malloc(sizeof(struct policy_definition));
    pd->conditions = malloc(sizeof(List));
    pd->actions = malloc(sizeof(List));

    list_init(pd->conditions);
    list_init(pd->actions);

    pd->conditions_met = 0;
    pd->actions_performed = 0;

    return pd;
}

void policy_definition_free(struct policy_definition *pd)
{
    list_destroy(pd->conditions);
    list_destroy(pd->actions);
    free(pd);
}

List *policy_definition_get_conditions(struct policy_definition *pd)
{
    return pd->conditions;
}

List *policy_definition_get_actions(struct policy_definition *pd)
{
    return pd->actions;
}

void policy_definition_put_action(struct policy_definition *pd, struct action *act)
{
    policy_definition_put(pd->actions, (void*)act);
}

void policy_definition_put_condition(struct policy_definition *pd, struct condition *condition)
{
    policy_definition_put(pd->conditions, (void*)condition);
}

void policy_defintiion_set_conditions_met(struct policy_definition *pd, uint8_t conditions_met)
{
    pd->conditions_met = conditions_met;
}

void policy_definition_set_actions_performed(struct policy_definition *pd, uint8_t actions_performed)
{
    pd->actions_performed = actions_performed;
}
