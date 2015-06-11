#ifndef __POLICY_HANDLER__
#define __POLICY_HANDLER__

#include "mptcp_state.h"
#include "mptcp_controller.h"
#include "route_enforcer.h"
#include "../resource_manager.h"
#include "../application_rules.h"

struct policy_handler_state * policy_handler_init(List *network_resources, List *application_specs);

typedef void * (*policy_handler_route_change_cb_t)
    (struct network_resource *nr, uint8_t mode, void* data);

typedef void * (*policy_handler_metric_change_cb_t)
    (struct network_resource *nr, uint8_t mode, void* data);

policy_handler_route_change_cb_t policy_handler_state_get_route_cb(struct policy_handler_state * ps);
policy_handler_metric_change_cb_t policy_handler_state_get_metric_cb(struct policy_handler_state * ps);

void policy_handler_state_set_route_cb(struct policy_handler_state * ps, policy_handler_route_change_cb_t cb, void *data);
void policy_handler_state_set_metric_cb(struct policy_handler_state * ps, policy_handler_metric_change_cb_t cb, void *data);
struct policy_handler_state * policy_handler_state_alloc(void);

#endif
