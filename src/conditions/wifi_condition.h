#include "../policy.h"

struct condition *parse_wifi_used_condition(void *key, void *value, void *comparator);
struct condition *parse_wifi_allowance_condition(void *key, void *value, void *comparator);
int init_wifi_bandwidth_condition(void);
