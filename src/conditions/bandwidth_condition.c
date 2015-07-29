#include <regex.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#include "../policy.h"
#include "../debug.h"
#include "../bandwidth_parser.h"
#include "../util.h"

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
    print_verb("Checking comparator is valid\n");
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

    print_debug("UNIT_STRING: %s ID: %d\n", unit_string, units);

    switch(units){
        case UNIT_BITS:
            break;
        case UNIT_KBITS:
            int_val *= 1024;
            break;
        case UNIT_MBITS:
            int_val *= 1048576;
            break;
        case UNIT_GBITS:
            int_val *= 1073741824;
            break;
    }

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
        print_verb("Looking for key: %s\n", key);
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

uint32_t read_bandwidth_file(void)
{
    static failure = 0;
    int fd;
    char line[512];
    ssize_t n;
    char * battery_voltage_file = "/tmp/bandwidth_consumption";

    memset(line, 0, 512);
    fd = open(battery_voltage_file, O_RDONLY);
    if (fd == -1){
        if(!failure)
            print_error("Failed to open bandwidth_consumption file\n");
        failure = 1;
        return 5.00;
    }

    n = read(fd, line, 512);
    if (n == -1){
        if(!failure)
            print_error("Failed to read from bandwidth_consumption file\n");
        failure = 1;
        return 5.00;
    }

    return atoi(line);
}

uint32_t get_bandwidth_consumption(void)
{
    return read_bandwidth_file();
}

void fire_callback(struct condition *c, condition_cb_t cb, void *data)
{
    if(cb && !condition_get_met(c)){
        //print_verb("Firing callback: %d\n", condition_get_met(c));
        condition_set_met(c, 1);
        cb(c, data);
    }
}


struct condition * check_bandwidth_condition(struct condition *c, uint32_t bandwidth, condition_cb_t cb, void *data)
{
    uint32_t comparator = condition_get_comparator(c);
    uint32_t *b = condition_get_value(c);

    switch(comparator){
        case COMPARATOR_LT:
            if(bandwidth < *b) {
                fire_callback(c, cb, data);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_GT:
            if(bandwidth > *b) {
                fire_callback(c, cb, data);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_GTE:
            if(bandwidth >= *b) {
                fire_callback(c, cb, data);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_LTE:
            if(bandwidth <= *b) {
                fire_callback(c, cb, data);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_EQUALS:
            if(*b == bandwidth) {
                fire_callback(c, cb, data);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_NEQUALS:
            if(*b != bandwidth) {
                fire_callback(c, cb, data);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
    }

    return (struct condition*)0;
}

struct condition * check_bandwidth_conditions(List *conditions, uint32_t bandwidth, condition_cb_t cb, void *data)
{
    Litem *item;
    list_for_each(item, conditions){
        struct condition *cond = (struct condition*)item->data;
        check_bandwidth_condition(cond, bandwidth, cb, data);
    }
    return (struct condition*)0;
}

void * start(void *context)
{
    List *conditions;
    struct condition_context *ctx = context;
    condition_cb_t cb;
    void *cb_data;

    cb = ctx->cb;
    cb_data = ctx->data;
    conditions = ctx->conditions;

    for(;;) {
        /*Battery Event Loop*/
        uint32_t bw = get_bandwidth_consumption();
        check_bandwidth_conditions(conditions, bw, cb, cb_data);
        sleep(1);
    }

    return 0;
}
