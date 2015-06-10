#include "../policy.h"

struct condition *parse_used_condition(void *key, void *value, void *comparator);
struct condition *parse_allowance_condition(void *key, void *value, void *comparator);
uint32_t get_current_reading(void);
float get_voltage_reading(void);
int init_bandwidth_condition(void);
