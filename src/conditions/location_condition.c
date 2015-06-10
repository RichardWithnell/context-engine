#include <time.h>
#include <unistd.h>

#include "../policy.h"
#include "../debug.h"
#include "location_condition.h"
#include "condition.h"

struct condition *parse_location_condition(void *key, void *value, void *comparator);
int init_location_condition(void);

static char *location_keys[] = { "location", 0 };
static parse_condition_t parsers[] = { parse_location_condition, 0 };


struct condition *parse_location_condition(void *key, void *value, void *comparator)
{
    struct condition *cond;

    cond = condition_alloc();

    condition_set_key(cond, (char*)key);

    return cond;
}

int init_location_condition(void)
{

    return 0;
}

struct condition *parse_condition(void *k, void *v, void *c)
{
    char *key = (char*)0;
    int i = 0;

    key = k;

    while (location_keys[i] != NULL) {
        if(!strcmp(key, location_keys[i])){
            return (parsers[i])(k, v, c);
        }
        i++;
    }

    return (struct condition*)0;
}

int init_condition(register_key_cb_t reg_cb, void *data)
{
    int i = 0;
    printf("Init Location Condition\n");
    while (location_keys[i] != NULL) {
        reg_cb(location_keys[i], data);
        i++;
    }
    return 0;
}

void * start(void *context)
{
//    struct condition_context *ctx = context;

    for(;;) {
        /*Location Event Loop*/
        sleep(1);
    }

    return 0;
}
