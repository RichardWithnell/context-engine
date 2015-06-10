#include <regex.h>
#include <time.h>
#include <unistd.h>

#include "../policy.h"
#include "../debug.h"
#include "../bandwidth_parser.h"

#include "bandwidth_condition.h"
#include "condition.h"

struct condition *parse_ssid_condition(void *key, void *value, void *comparator);
struct condition *parse_allowance_condition(void *key, void *value, void *comparator);

static char *bandwidth_keys[] = { "bandwidth_allowance", "ssid", 0 };
static parse_condition_t parsers[] = { parse_allowance_condition, parse_ssid_condition, 0 };



struct condition *parse_ssid_condition(void *k, void *v, void *c)
{
    static int valid_comparators[] = { COMPARATOR_EQUALS, COMPARATOR_NEQUALS, -1 };

    struct condition *cond;
    char *value = (char*)0;
    char *comparator = (char*)0;
    int comparator_id = 0;

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

    condition_copy_value(cond, value, strlen(value) + 1);
    condition_set_comparator(cond, comparator_id);
    condition_set_unit(cond, UNIT_UNSPEC);
    condition_set_key(cond, (char*)k);


    return cond;
}

struct condition *parse_allowance_condition(void *k, void *v, void *c)
{
    static int valid_comparators[] = { COMPARATOR_LT, COMPARATOR_GT,
                                    COMPARATOR_GTE, COMPARATOR_LTE,
                                    COMPARATOR_EQUALS, COMPARATOR_NEQUALS, -1 };

    int comparator_id = 0;
    struct condition *cond = (struct condition*)0;
    char *value = (char*)0;
    char *comparator = (char*)0;
    char *unit_string = (char*)0;
    uint32_t int_val = 0;
    uint32_t units = 0;

    cond = condition_alloc();
    if(!cond){
        return (struct condition*)0;
    }

    comparator = (char*)c;
    value = (char*)v;

    comparator_id = find_comparator_id(comparator);
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

    condition_set_comparator(cond, comparator_id);

    if(validate_bandwidth_value(value)){
        print_error("Failed to validate value: \"%s\" for key %s\n",
          value, (char*)k);
        condition_free(cond);
        return (struct condition*)0;
    }

    int_val = get_bandwidth(value);
    unit_string = get_bandwidth_unit(value);
    units = find_unit_id(unit_string);

    condition_copy_value(cond, &int_val, sizeof(int_val));
    condition_set_unit(cond, units);
    condition_set_key(cond, (char*)k);

    return cond;
}

struct condition *parse_condition(void *k, void *v, void *c)
{
    char *key = (char*)0;
    int i = 0;

    key = k;

    while (bandwidth_keys[i] != NULL) {
        if(!strcmp(key, bandwidth_keys[i])){
            print_verb("Using Parser: %s\n", bandwidth_keys[i]);
            return (parsers[i])(k, v, c);
        }
        i++;
    }

    return (struct condition*)0;
}

int init_ssid_condition(void)
{
    return 0;
}

int init_bandwidth_condition(void)
{
    return 0;
}


int init_condition(register_key_cb_t reg_cb, void *data)
{
    int i = 0;

    printf("Init Bandwidth Condition\n");

    init_bandwidth_condition();
    init_ssid_condition();

    while (bandwidth_keys[i] != NULL) {
        reg_cb(bandwidth_keys[i], data);
        i++;
    }

    return 0;
}

void * start(void *context)
{
//    struct condition_context *ctx = context;

    for(;;) {
        /*Bandwidth Event Loop*/
        sleep(1);
    }

    return 0;
}
