#include "../policy.h"


struct condition *parse_capacity_condition(void *key, void *value, void *comparator);
struct condition *parse_voltage_condition(void *key, void *value, void *comparator);
int init_battery_condition(void);
