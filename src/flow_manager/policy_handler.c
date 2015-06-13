#include <pthread.h>

#include "policy_handler.h"
#include "../debug.h"

struct policy_handler_state {
    struct mptcp_state *mp_state;
    List *network_resources;
    List *application_specs;
    List *iptables_rules;
    policy_handler_route_change_cb_t route_change_cb;
    void * route_change_data;
    policy_handler_metric_change_cb_t metric_change_cb;
    void * metric_change_data;
    pthread_t mptcp_thread;
};

List *policy_handler_state_get_rules(struct policy_handler_state *ph)
{
    return ph->iptables_rules;
}

int policy_handler_create_subflows_for_connection(struct mptcp_state *state, struct connection *conn, List *network_resources){
    Litem *item;
    uint32_t address = 0;
    uint32_t loc_id = 0;
    int ret = 0;

    list_for_each(item, network_resources){
        struct network_resource *nr = (struct network_resource *)item->data;
        address = network_resource_get_address(nr);
        loc_id = network_resource_get_index(nr);
        if( network_resource_get_available(nr) == RESOURCE_AVAILABLE
          && network_resource_get_multipath(nr) == RULE_MULTIPATH_ENABLED
          && address != conn->saddr){
            ret = mptcp_create_subflow(state, address, loc_id, conn);
            if(!ret){
                mptcp_connection_add_subflow(conn, address, loc_id);
            } else {
                print_error("Create subflow failed\n");
            }
        }
    }

    return ret;
}


int policy_handler_add_mptcp_connection(struct mptcp_state *mp_state, struct connection *conn, List *network_resources)
{
    conn->subflows = malloc(sizeof(List));
    list_init(conn->subflows);
    mptcp_state_lock(mp_state);
    mptcp_state_put_connection(mp_state, conn);
    mptcp_state_unlock(mp_state);

    if(conn->multipath == RULE_MULTIPATH_ENABLED){
        policy_handler_create_subflows_for_connection(mp_state, conn, network_resources);
    }

    return 0;
}

int policy_handler_del_mptcp_connection(struct mptcp_state *mp_state, struct connection *conn, void *data)
{
    mptcp_state_lock(mp_state);
    conn = mptcp_state_pop_connection_by_token(mp_state, conn->token);
    mptcp_state_unlock(mp_state);

    list_destroy(conn->subflows);
    free(conn);

    return 0;
}

void *policy_handler_mptcp_cb(struct mptcp_state *mp_state, struct connection *conn, uint8_t mode, void *data)
{
    if(mode == MPTCP_REM_CONN){
        print_verb("MPTCP Event: Remove Connection (%d)\n", mode);
        policy_handler_del_mptcp_connection(mp_state, conn, data);
    } else if(mode == MPTCP_NEW_CONN) {
        print_verb("MPTCP Event: Add Connection (%d)\n", mode);
        policy_handler_add_mptcp_connection(mp_state, conn, data);
    } else {
        print_error("MPTCP Event: Unknown Code (%d)\n", mode);
    }

    return (void*)0;
}

void phadd_route_cb(struct mptcp_state *state, struct connection *conn, void* data)
{
    struct network_resource *nr;
    int ret = 0;
    nr = (struct network_resource *)data;

    if(conn->multipath == RULE_MULTIPATH_ENABLED){
        ret = mptcp_create_subflow(state, network_resource_get_address(nr), network_resource_get_index(nr), conn);
        if(!ret){
            mptcp_connection_add_subflow(conn, network_resource_get_address(nr), network_resource_get_index(nr));
        }
    }
}

int policy_handler_add_route_cb(struct network_resource *nr, struct policy_handler_state *ps)
{
    struct mptcp_state *mp_state;
    print_verb("\n");

    if(network_resource_get_multipath(nr) != RULE_MULTIPATH_ENABLED){
        print_debug("Not using resource, multipath not enabled\n");
        return -1;
    }

    if(network_resource_get_available(nr) != RESOURCE_AVAILABLE){
        print_debug("Not using resource, soft set to unavailable\n");
        return -1;
    }

    mp_state = ps->mp_state;
    mptcp_for_each_connection(mp_state, phadd_route_cb, nr);

    return 0;
}

void phdel_route_cb(struct mptcp_state *state, struct connection *connection, void* data)
{
    struct network_resource *nr;
    uint32_t address = 0;

    nr = (struct network_resource *)data;

    address = network_resource_get_address(nr);

    if(connection->multipath == RULE_MULTIPATH_ENABLED){
        if(connection->saddr == address){
            mptcp_remove_subflow(state, address, network_resource_get_index(nr), connection);
        } else {
            int idx = 0;
            Litem *item;

            list_for_each(item, connection->subflows) {
                struct subflow *sf = (struct subflow*)item->data;
                if(sf->saddr == address){
                    mptcp_remove_subflow(state, address, network_resource_get_index(nr), connection);
                    list_remove(connection->subflows, idx);
                    free(item);
                    print_verb("Deleted network_resource from list\n");
                    return;
                }
                idx++;
            }
        }
    }
}


int policy_handler_del_route_cb(struct network_resource *nr, struct policy_handler_state *ps)
{
    print_verb("\n");
    struct mptcp_state *mp_state;

    mp_state = ps->mp_state;

    mptcp_for_each_connection(mp_state, phdel_route_cb, nr);

    return 0;
}

void * policy_handler_route_change_cb(struct network_resource *nr, uint8_t mode, void* data)
{
    if(mode == LINK_EVENT_UP){
        print_verb("Link Event: UP\n");
        policy_handler_add_route_cb(nr, data);
    } else if(mode == LINK_EVENT_DOWN){
        print_verb("Link Event: DOWN\n");
        policy_handler_del_route_cb(nr, data);
    } else {
        print_error("Link Event: Unknown code (%d)\n", mode);
    }

    return (void*)0;
}

void * policy_handler_metric_change_cb(struct policy_handler_state *ph_state, void* data)
{
    route_selector(ph_state->network_resources,
        ph_state->application_specs,
        ph_state->iptables_rules);

    return 0;
}

struct policy_handler_state * policy_handler_init(List *network_resources, List *app_specs)
{
    struct policy_handler_state *ph_state;
    struct mptcp_state *mp_state = (struct mptcp_state*)0;
    pthread_t mptcp_thread;

    ph_state = policy_handler_state_alloc();

    ph_state->network_resources = network_resources;
    ph_state->application_specs = app_specs;

    policy_handler_state_set_route_cb(ph_state, policy_handler_route_change_cb, 0);
    policy_handler_state_set_metric_cb(ph_state, policy_handler_metric_change_cb, 0);

    if(mptcp_check_local_capability()){
        print_debug("Host is MP-Capable");
        mp_state = mptcp_state_alloc();

        mptcp_state_set_event_cb(mp_state, policy_handler_mptcp_cb, network_resources);

        pthread_create(&mptcp_thread, 0,mptcp_control_start, mp_state);
    } else {
        print_error("Host is not MP-Capable\n");
    }

    init_iptables_context();

    return ph_state;
}

policy_handler_route_change_cb_t policy_handler_state_get_route_cb(struct policy_handler_state * ps)
{
    return ps->route_change_cb;
}

policy_handler_metric_change_cb_t policy_handler_state_get_metric_cb(struct policy_handler_state * ps)
{
    return ps->metric_change_cb;
}

void policy_handler_state_set_route_cb(struct policy_handler_state * ps, policy_handler_route_change_cb_t cb, void *data)
{
    ps->route_change_cb = cb;
    ps->route_change_data = data;
}

void policy_handler_state_set_metric_cb(struct policy_handler_state * ps, policy_handler_metric_change_cb_t cb, void *data)
{

    ps->metric_change_cb = cb;
    ps->metric_change_data = data;
}

struct policy_handler_state * policy_handler_state_alloc(void)
{
    struct policy_handler_state *ph;
    ph = malloc(sizeof(struct policy_handler_state));
    ph->iptables_rules = malloc(sizeof(List));
    list_init(ph->iptables_rules);
    return ph;
}
