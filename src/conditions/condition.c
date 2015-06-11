#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <arpa/inet.h>

#include <linux/if.h>

#include "condition.h"

#define KEY_LENGTH 64

struct condition
{
    char key[KEY_LENGTH];
    char link_name[IFNAMSIZ];
    void * value;
    uint32_t unit;
    uint32_t comparator;
    uint32_t condition_id;
    uint8_t condition_met;
    struct policy_definition *parent;
    condition_cb_t callback;
    void *cb_data;
};

static const char * comparators[__COMPARATOR_MAX] = {
    [COMPARATOR_UNSPEC] = "",
    [COMPARATOR_EQUALS] = "==",
    [COMPARATOR_NEQUALS] = "!=",
    [COMPARATOR_LT] = "<",
    [COMPARATOR_GT] = ">",
    [COMPARATOR_LTE] = "<=",
    [COMPARATOR_GTE] = ">=",
    [COMPARATOR_IN] = "in",
    [COMPARATOR_OUT] = "out",
};

int find_comparator_id(char *comp)
{
    int i = 0;
    for(; i < __COMPARATOR_MAX; i++){
        if(!strcmp(comparators[i], comp)){
            return i;
        }
    }
    return -1;
}

static inline int sizeof_condition(void)
{
    return sizeof(struct condition);
}

void condition_print(struct condition *c)
{
    printf("Key: %s\n", c->key);
    printf("Value: %p\n", c->value);
    printf("Comparator: %s\n", comparators[c->comparator]);
}

struct condition * condition_alloc(void)
{
    struct condition *c = (struct condition*)0;
    c = malloc(sizeof(struct condition));

    memset(c->key, 0, KEY_LENGTH);
    c->value = 0;
    c->unit = 0;
    c->comparator = 0;
    c->condition_id = 0;
    c->parent = (struct policy_definition*)0;
    c->callback = (condition_cb_t)0;
    c->cb_data = (void *)0;
    c->condition_met = 0;

    return c;
}

void condition_free(struct condition *c)
{
    free(c);
}

void condition_set_parent(struct condition *c, struct policy_definition *pd)
{
    c->parent = pd;
}


struct policy_definition * condition_get_parent(struct condition *c)
{
    return c->parent;
}

char * condition_get_key(struct condition *c)
{
    return c->key;
}

char * condition_get_link_name(struct condition *c)
{
    return c->link_name;
}

uint8_t condition_get_met(struct condition *c)
{
    return c->condition_met;
}

void condition_set_met(struct condition *c, uint8_t met)
{
    c->condition_met = met;
}

void condition_set_link_name(struct condition *c, char * name)
{
    strncpy(c->link_name, name, strlen(name));
}


void condition_set_key(struct condition *c, char * key)
{
    strncpy(c->key, key, strlen(key));
}

void * condition_get_value(struct condition *c){
    return c->value;
}

void condition_copy_value(struct condition *c, void* value, uint32_t length)
{
    c->value = malloc(length);
    memcpy(c->value, value, length);
}


void condition_set_value(struct condition *c, void* value)
{
    c->value = value;
}

void condition_set_unit(struct condition *c, uint32_t unit)
{
    c->unit = unit;
}

uint32_t condition_get_comparator(struct condition *c)
{
    return c->comparator;
}

void condition_set_comparator(struct condition *c, uint32_t comparator)
{
    c->comparator = comparator;
}

condition_cb_t condition_get_callback(struct condition *c)
{
    return c->callback;
}

void condition_set_callback(struct condition *c, condition_cb_t callback )
{
    c->callback = callback;
}
