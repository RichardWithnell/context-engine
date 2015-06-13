#ifndef FLOW_MANAGER_ROUTE
#define FLOW_MANAGER_ROUTE



int init_iptables_context(void);
int route_selector(List * resource_list, List * app_specs, List * iptables_rules);

#endif
