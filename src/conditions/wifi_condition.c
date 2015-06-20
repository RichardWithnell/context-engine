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

#include "bandwidth_condition.h"
#include "condition.h"

struct condition *parse_wifi_ssid_condition(void *key, void *value, void *comparator);
struct condition *parse_wifi_allowance_condition(void *key, void *value, void *comparator);
struct condition *parse_wifi_connected_condition(void *key, void *value, void *comparator);

static char *wifi_keys[] = { "wifi_bandwidth_allowance", "wifi_ssid", "wifi_connected", 0 };
static parse_condition_t parsers[] = { parse_wifi_allowance_condition, parse_wifi_ssid_condition, parse_wifi_connected_condition, 0 };

struct condition *parse_wifi_connected_condition(void *k, void *v, void *c)
{
    static int valid_comparators[] = { COMPARATOR_EQUALS, COMPARATOR_NEQUALS, -1 };

    struct condition *cond = (struct condition*)0;
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


struct condition *parse_wifi_ssid_condition(void *k, void *v, void *c)
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

struct condition *parse_wifi_allowance_condition(void *k, void *v, void *c)
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

    while (wifi_keys[i] != NULL) {
        if(!strcmp(key, wifi_keys[i])){
            print_verb("Using Parser: %s\n", wifi_keys[i]);
            return (parsers[i])(k, v, c);
        }
        i++;
    }

    return (struct condition*)0;
}

char * read_ssid_file(char * linkname)
{
    int fd;
    char file[512];
    ssize_t n;

    char *ssid = (char*)0;

    ssid = malloc(128);

    memset(ssid, 0, 128);
    memset(file, 0, 512);

    print_verb("Reading file for: %s\n", linkname);
    sprintf(file, "/tmp/ssid_%s", linkname);

    fd = open(file, O_RDONLY);
    if (fd == -1){
         print_error("Failed to open ssid file\n");
         free(ssid);
         return (char*)0;
    }

    n = read(fd, ssid, 128);
    if (n == -1){
        print_error("Failed to read from ssid file\n");
        free(ssid);
        return (char*)0;
    }

    return ssid;
}

char* get_ssid(char *linkname)
{
    return read_ssid_file(linkname);
}

void fire_callback(struct condition *c, condition_cb_t cb, void *data)
{
    if(cb && !condition_get_met(c)){
        //print_verb("Firing callback: %d\n", condition_get_met(c));
        condition_set_met(c, 1);
        cb(c, data);
    }
}

int init_wifi_ssid_condition(void)
{
    return 0;
}

int init_wifi_bandwidth_condition(void)
{
    return 0;
}


int init_condition(register_key_cb_t reg_cb, void *data)
{
    int i = 0;

    printf("Init Bandwidth Condition\n");

    init_wifi_bandwidth_condition();
    init_wifi_ssid_condition();

    while (wifi_keys[i] != NULL) {
        reg_cb(wifi_keys[i], data);
        i++;
    }

    return 0;
}

struct condition * check_ssid_condition(struct condition *c, char *ssid, condition_cb_t cb, void *data)
{
    uint32_t comparator = condition_get_comparator(c);
    char * s = condition_get_value(c);

    switch(comparator){
        case COMPARATOR_EQUALS:
            if(!strcmp(s, ssid)) {
                fire_callback(c, cb, data);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
        case COMPARATOR_NEQUALS:
            if(strcmp(s, ssid)) {
                fire_callback(c, cb, data);
            } else if(condition_get_met(c)){
                condition_set_met(c, 0);
            }
            break;
    }

    return (struct condition*)0;
}

struct condition * check_ssid_conditions(List *conditions, condition_cb_t cb, void *data)
{
    Litem *item;
    list_for_each(item, conditions){
        struct condition *cond = (struct condition*)item->data;
        char *ssid = get_ssid(condition_get_link_name(cond));
        if(ssid){
            check_ssid_condition(cond, "wlan0", cb, data);
            free(ssid);
        }
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
        check_ssid_conditions(conditions, cb, cb_data);
        sleep(1);
    }

    return 0;
}
