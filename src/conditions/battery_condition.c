#include <time.h>
#include <unistd.h>

#include "../policy.h"

#include "battery_condition.h"
#include "condition.h"

struct condition *parse_voltage_condition(void *key, void *value, void *comparator);
struct condition *parse_capacity_condition(void *key, void *value, void *comparator);


static char *battery_keys[] = { "battery_capacity", "battery_voltage", 0};
static parse_condition_t parsers[] = {parse_capacity_condition, parse_voltage_condition, 0 };

struct condition *parse_capacity_condition(void *key, void *value, void *comparator)
{
    struct condition *c;

    c = condition_alloc();

    return c;
}

struct condition *parse_voltage_condition(void *k, void *v, void *c)
{
    static int valid_comparators[] = { COMPARATOR_LT, COMPARATOR_GT,
                                    COMPARATOR_GTE, COMPARATOR_LTE,
                                    COMPARATOR_EQUALS, COMPARATOR_NEQUALS, -1 };
    struct condition *cond;
    char *value = (char*)0;
    char *comparator = (char*)0;
    char *end = (char*)0;
    int comparator_id = 0;
    float voltage = (float)0;

    if(!v){
        print_error("Value variable is null\n");
        return (struct condition*)0;
    }

    if(!c){
        print_error("Comparator variable is null\n");
        return (struct condition*)0;
    }

    cond = condition_alloc();
    if(!cond){
        print_error("malloc failed\n");
        return (struct condition*)0;
    }

    comparator = (char*)c;
    value = (char*)v;

    comparator_id = find_comparator_id(comparator);

    print_verb("Found comparator id\n");

    if(comparator_id < 0) {
        print_error("Invalid Comparator: %s\n", comparator);
        condition_free(cond);
        return (struct condition*)0;
    }

    if(condition_check_valid_comparator(valid_comparators, comparator_id)) {
        print_error("Comparator: %s does not make sense for condition %s\n",
          comparator, (char*)k);
        condition_free(cond);
        return (struct condition*)0;
    }

    print_verb("Checke Comparator is valid\n");

    voltage = strtof(value, &end);
    if(voltage == 0){
        print_error("Failed to convert voltage parameter\n");
        return (struct condition*)0;
    }
    printf("Value: %s : %f\n", value, voltage);

    condition_copy_value(cond, &voltage, sizeof(float));

    printf("Value: %f\n", *(float*)condition_get_value(cond));

    condition_set_comparator(cond, comparator_id);
    condition_set_unit(cond, UNIT_UNSPEC);
    condition_set_key(cond, (char*)k);

    return cond;
}

struct condition *parse_condition(void *k, void *v, void *c)
{
    char *key = (char*)0;
    int i = 0;

    key = k;

    while (battery_keys[i] != NULL) {
        if(!strcmp(key, battery_keys[i])){
            return (parsers[i])(k, v, c);
        }
        i++;
    }

    return (struct condition*)0;
}

int init_condition(register_key_cb_t reg_cb, void *data)
{
    int i = 0;

    printf("Init Battery Condition\n");

    while (battery_keys[i] != NULL) {
        reg_cb(battery_keys[i], data);
        i++;
    }

    return 0;
}

float get_battery_voltage(void)
{
    float r = drand48() + 3.30f;
    return r;
}

void fire_callback(struct condition *c, condition_cb_t cb)
{
    if(cb && !condition_get_met(c)){
        print_verb("Firing callback: %d\n", condition_get_met(c));
        condition_set_met(c, 1);
        cb(c, 0);
    }
}

struct condition * check_voltage_condition(struct condition *c, float voltage, condition_cb_t cb)
{
    uint32_t comparator = condition_get_comparator(c);
    float *v = condition_get_value(c);


    switch(comparator){
        case COMPARATOR_LT:
            //print_debug("Voltage?: %f < %f\n", voltage, *v);
            if(voltage < *v) {
                fire_callback(c, cb);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_GT:
            if(voltage > *v) {
                fire_callback(c, cb);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_GTE:
            if(voltage >= *v) {
                fire_callback(c, cb);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_LTE:
            if(voltage <= *v) {
                fire_callback(c, cb);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_EQUALS:
            if(*v == voltage) {
                fire_callback(c, cb);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_NEQUALS:
            if(*v != voltage) {
                fire_callback(c, cb);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
    }

    return (struct condition*)0;
}

struct condition * check_voltage_conditions(List *conditions, float voltage, condition_cb_t cb)
{
    Litem *item;
    list_for_each(item, conditions){
        struct condition *cond = (struct condition*)item->data;
        check_voltage_condition(cond, voltage, cb);
    }
    return (struct condition*)0;
}

void * start(void *context)
{
    List *conditions;
    struct condition_context *ctx = context;
    condition_cb_t cb;

    cb = ctx->cb;
    conditions = ctx->conditions;

    srand48(time(NULL));

    for(;;) {
        /*Battery Event Loop*/
        float voltage = get_battery_voltage();
        check_voltage_conditions(conditions, voltage, cb);
        sleep(1);
    }

    return 0;
}
