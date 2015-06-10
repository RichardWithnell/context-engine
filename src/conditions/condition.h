#ifndef CONTEXT_CONDITION
#define CONTEXT_CONDITION

#include <stdint.h>

#include "../list.h"
#include "../bandwidth_parser.h"

enum {
    KEY_UNSPEC = 0x00,
    KEY_BANDWIDTH_ALLOWANCE = 0x01,
    KEY_SIGNAL_STRENGTH = 0x02,
    KEY_CPU_USAGE = 0x03,
    KEY_LINK_COST = 0x04,
    KEY_LOCATION = 0x05,
    KEY_APPLICATION = 0x06,
    KEY_LINK_AVAILABILITY = 0x07,
    KEY_BATTERY_CAPACITY = 0x08,
    KEY_BATTERY_VOLTAGE = 0x09,
    KEY_SSID = 0x0A,
    __KEY_MAX = 0x0B,
};

enum {
    COMPARATOR_UNSPEC = 0x00,
    COMPARATOR_EQUALS = 0x01,
    COMPARATOR_NEQUALS = 0x02,
    COMPARATOR_LT = 0x03,
    COMPARATOR_GT = 0x04,
    COMPARATOR_LTE = 0x05,
    COMPARATOR_GTE = 0x06,
    COMPARATOR_IN = 0x07,
    COMPARATOR_OUT = 0x08,
    __COMPARATOR_MAX = 0x09,
};



struct condition;
struct policy_definition;
struct condition_context;

typedef void * (*start_context_lib_t)(void *context);
typedef int (*register_key_cb_t)(char *key, void *data);
typedef int (*init_condition_t)(register_key_cb_t reg_cb, void *data);
typedef void (*condition_cb_t)(struct condition *c, void *data);
typedef struct condition* (*parse_condition_t)(void *key, void *value, void *comparator);

void * start(void *context);
int init_condition(register_key_cb_t reg_cb, void *data);
struct condition *parse_condition(void *key, void *value, void *comparator);

struct condition * condition_alloc(void);
void condition_free(struct condition *c);

void condition_set_link_name(struct condition *c, char *name);
char * condition_get_link_name(struct condition *c);

void condition_set_key(struct condition *c, char * key);
char * condition_get_key(struct condition *c);

uint8_t condition_get_met(struct condition *c);
void condition_set_met(struct condition *c, uint8_t met);

void condition_copy_value(struct condition *c, void* value, uint32_t length);
void condition_set_value(struct condition *c, void * value);
void * condition_get_value(struct condition *c);

void condition_set_unit(struct condition *c, uint32_t unit);
void condition_set_comparator(struct condition *c, uint32_t comparator);
uint32_t condition_get_comparator(struct condition *c);

condition_cb_t condition_get_callback(struct condition *c);
void condition_set_callback(struct condition *c, condition_cb_t callback);

void condition_print(struct condition *c);

int find_comparator_id(char *comp);

struct condition_context
{
    condition_cb_t cb;
    List *conditions;
};

static inline int condition_check_valid_comparator(int array[], int comparator)
{
    int i = 0;
    while (array[i] != -1){
        if(array[i] == comparator){
            return 0;
        }
    }
    return -1;
}

#endif
