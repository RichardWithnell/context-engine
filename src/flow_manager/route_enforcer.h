#ifndef FLOW_MANAGER_ROUTE
#define FLOW_MANAGER_ROUTE

#include "../application_rules.h"
#include "../resource_manager.h"

int init_iptables_context(void);
int install_iptables_rule(struct network_resource *nr, struct application_spec *as, List * iptables_rules);
int route_selector(List * resource_list, List * app_specs, List * iptables_rules);
struct network_resource * route_selector_resource_loop(List * resource_list, struct application_spec *spec, int try_backup);

#endif
