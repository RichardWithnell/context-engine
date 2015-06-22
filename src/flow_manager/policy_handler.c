#include <pthread.h>

#include "policy_handler.h"
#include "../iptables/iptables.h"
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

List *policy_handler_state_get_resources(struct policy_handler_state *ph)
{
    return ph->network_resources;
}

List *policy_handler_state_get_specs(struct policy_handler_state *ph)
{
    return ph->application_specs;
}

List *policy_handler_state_get_rules(struct policy_handler_state *ph)
{
    return ph->iptables_rules;
}

int policy_handler_create_subflows_for_connection(struct mptcp_state *state, struct connection *conn, List *network_resources, int try_backup){
    Litem *item;
    uint32_t address = 0;
    uint32_t loc_id = 0;
    int ret = 0;

    list_for_each(item, network_resources){
        struct network_resource *nr = (struct network_resource *)item->data;
        address = network_resource_get_address(nr);
        loc_id = network_resource_get_loc_id(nr);
        if(network_resource_get_available(nr) == RESOURCE_UNAVAILABLE) {
            continue;
        } else if(network_resource_get_available(nr) == RESOURCE_BACKUP
                  && try_backup == 0)
        {
            continue;
        }

        if( network_resource_get_multipath(nr) == RULE_MULTIPATH_ENABLED
            && address != conn->saddr)
        {
            print_debug("Found valid interface for new subflow: %s (MP Enabled)\n", network_resource_get_ifname(nr));

            print_verb("Address of network Resource: %s\n", ip_to_str(htonl(address)));
            print_verb("Address of new connection:   %s\n", ip_to_str(htonl(conn->saddr)));

            ret = mptcp_create_subflow(state, address, loc_id, conn);
            if(!ret){
                mptcp_connection_add_subflow(conn, address, loc_id, (void*)nr);
            } else {
                print_error("Create subflow failed\n");
            }
        }
    }

    return ret;
}


int policy_handler_add_mptcp_connection(struct mptcp_state *mp_state, struct connection *conn, void *data)
{
    Litem *item = (Litem*)0;
    List *network_resources = (List*)0;
    List *application_specs = (List*)0;
    struct policy_handler_state *ph_state = (struct policy_handler_state*)0;

    print_debug("Add MPTCP Connection: %s:%d\n", ip_to_str(htonl(conn->daddr)), conn->dport);

    ph_state = (struct policy_handler_state *)data;
    network_resources = ph_state->network_resources;
    application_specs = ph_state->application_specs;
    conn->multipath = RULE_MULTIPATH_ENABLED;

    if(!(conn->subflows)){
        print_error("Failed to malloc subflow list for connection\n");
        return -1;
    }

    mptcp_state_lock(mp_state);
    mptcp_state_put_connection(mp_state, conn);
    mptcp_state_unlock(mp_state);

    print_verb("Finding MP Capability\n");
    list_for_each(item, application_specs){
        struct application_spec *spec = (struct application_spec*)0;
        spec = (struct application_spec*)item->data;

        if(spec && application_spec_get_daddr(spec) == conn->daddr
                && application_spec_get_dport(spec) == conn->dport)
        {
            conn->multipath = application_spec_get_multipath(spec);
            print_debug("Found App Spec for new Connection\n");
            print_debug("\t setting MP capability: %d\n", application_spec_get_multipath(spec));
            break;
        }
    }

    item = (Litem*)0;
    print_verb("Setting resource for MPTCP Connection\n");
    list_for_each(item, network_resources){
        struct network_resource *netres = (struct network_resource*)0;
        netres = (struct network_resource*)item->data;
        if(netres && network_resource_get_address(netres) == conn->saddr){
            conn->resource = netres;
            print_verb("Set Resource for MPTCP Connection\n");
            break;
        }
    }

    if(conn->multipath == RULE_MULTIPATH_ENABLED){
        print_verb("Connection is multipath capable, create new subflows\n");
        policy_handler_create_subflows_for_connection(mp_state, conn, network_resources, 0);
    } else {
        print_verb("Connection isn't multipath capable, don't create new subflows\n");
    }

    return 0;
}

int policy_handler_del_mptcp_connection(struct mptcp_state *mp_state, struct connection *conn, void *data)
{
    print_debug("Delete MPTCP Connection\n");

    mptcp_state_lock(mp_state);
    conn = mptcp_state_pop_connection_by_token(mp_state, conn->token);
    mptcp_state_unlock(mp_state);

    if(conn){
        list_destroy(conn->subflows);
        free(conn);
    }

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

        ret = mptcp_create_subflow(state, network_resource_get_address(nr), network_resource_get_loc_id(nr), conn);
        if(!ret){
            mptcp_connection_add_subflow(conn, network_resource_get_address(nr), network_resource_get_loc_id(nr), nr);
        }


    } else if(conn->multipath == RULE_MULTIPATH_HANDOVER){
        /*Check there are other available subflows*/
        if(list_size(conn->subflows) == 0 && conn->active == 0){
            /*No other subflows currently active*/
            ret = mptcp_create_subflow(state, network_resource_get_address(nr), network_resource_get_loc_id(nr), conn);
            if(!ret){
                mptcp_connection_add_subflow(conn, network_resource_get_address(nr), network_resource_get_loc_id(nr), nr);
            }
        }
    }
}

int policy_handler_add_route_cb(struct network_resource *nr, struct policy_handler_state *ps)
{
    char addr_str[32];
    struct mptcp_state *mp_state;
    print_verb("\n");

    memset(addr_str, 0, 32);

    if(!nr || !ps) {
        print_error("Parameters cannot be null\n");
        return -1;
    }

    if(network_resource_get_multipath(nr) == RULE_MULTIPATH_DISABLED){
        print_error("Not using resource, multipath not enabled\n");
        return -1;
    }

    if(network_resource_get_available(nr) == RESOURCE_UNAVAILABLE){
        print_error("Not using resource, soft set to unavailable\n");
        return -1;
    }

    strcpy(addr_str, ip_to_str(htonl(network_resource_get_address(nr))));
    iptables_add_snat(addr_str, network_resource_get_table(nr));

    mp_state = ps->mp_state;
    mptcp_for_each_connection(mp_state, phadd_route_cb, nr);

    return 0;
}

struct phdel_route_cb_data {
    struct network_resource *nr;
    struct policy_handler_state *ps;
};

void phdel_route_cb(struct mptcp_state *state, struct connection *connection, void* data)
{
    struct policy_handler_state *ps;
    struct network_resource *nr;
    uint32_t address = 0;
    struct phdel_route_cb_data *cb_data;

    cb_data = (struct phdel_route_cb_data*)data;

    ps = (struct policy_handler_state *)cb_data->ps;
    nr = (struct network_resource *)cb_data->nr;

    address = network_resource_get_address(nr);

    if(connection->multipath == RULE_MULTIPATH_ENABLED){
        if(connection->saddr == address){
            connection->active = 0;
            connection->resource = 0;
            mptcp_remove_subflow(state, address, network_resource_get_loc_id(nr), connection);
        } else {
            int idx = 0;
            Litem *item;

            list_for_each(item, connection->subflows) {
                struct subflow *sf = (struct subflow*)item->data;
                if(sf->saddr == address){
                    mptcp_remove_subflow(state, address, network_resource_get_loc_id(nr), connection);
                    list_remove(connection->subflows, idx);
                    free(item);
                    print_verb("Deleted network_resource from list\n");
                    break;
                }
                idx++;
            }
        }

        if((connection->active + list_size(connection->subflows)) == 0){
            policy_handler_create_subflows_for_connection(state, connection, ps->network_resources, 1);
        }

    } else if(connection->multipath == RULE_MULTIPATH_HANDOVER){
        /*Select new route*/
        int ret = 0;
        struct network_resource *candidate = (struct network_resource*)0;
        struct application_spec *spec = (struct application_spec*)0;
        Litem *item = (Litem*)0;

        list_for_each(item, ps->application_specs){
            struct application_spec  *tmp = item->data;
            if(connection->daddr == application_spec_get_daddr(spec)
              && connection->dport == application_spec_get_dport(spec))
            {
                spec = tmp;
                break;
            }
        }

        candidate = route_selector_resource_loop(ps->network_resources, spec, 0);
        if(!candidate) {
            route_selector_resource_loop(ps->network_resources, spec, 1);
        }

        if(candidate){
            print_log("Chosen Link %s (%zu)\n",
                network_resource_get_ifname(candidate),
                htonl(network_resource_get_address(candidate)));

            print_log("\tfor App Spec: %zu:%d\n",
                htonl(application_spec_get_daddr(spec)),
                application_spec_get_dport(spec));

            install_iptables_rule(candidate, spec, ps->iptables_rules);

            ret = mptcp_create_subflow(state,
                    network_resource_get_address(nr),
                    network_resource_get_loc_id(nr),
                    connection);

            if(!ret){
                mptcp_connection_add_subflow(connection,
                    network_resource_get_address(nr),
                    network_resource_get_loc_id(nr), nr);
            }
        }

        /*Remove old address*/
        if(connection->saddr == address){
            mptcp_remove_subflow(state, address,
                network_resource_get_loc_id(nr),
                connection);
            connection->active = 0;
            connection->resource = 0;
        } else {
            int idx = 0;
            Litem *item;

            list_for_each(item, connection->subflows) {
                struct subflow *sf = (struct subflow*)item->data;
                if(sf->saddr == address){
                    mptcp_remove_subflow(state,
                        address,
                        network_resource_get_loc_id(nr),
                        connection);

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
    char addr_str[32];
    print_verb("\n");
    struct mptcp_state *mp_state;
    struct phdel_route_cb_data data;

    memset(addr_str, 0, 32);

    data.nr = nr;
    data.ps = ps;

    mp_state = ps->mp_state;

    mptcp_for_each_connection(mp_state, phdel_route_cb, &data);

    strcpy(addr_str, ip_to_str(htonl(network_resource_get_address(nr))));
    iptables_del_snat(addr_str, network_resource_get_table(nr));

    route_selector(ps->network_resources,
        ps->application_specs,
        ps->iptables_rules);

    return 0;
}

void * policy_handler_route_change_cb(struct network_resource *nr, uint8_t mode, void* data)
{
    if(mode == LINK_EVENT_UP){
        print_verb("Link Event: UP\n");
        policy_handler_add_route_cb(nr, (struct policy_handler_state *)data);
    } else if(mode == LINK_EVENT_DOWN){
        print_verb("Link Event: DOWN\n");
        policy_handler_del_route_cb(nr, (struct policy_handler_state *)data);
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
        print_debug("Host is MP-Capable\n");
        mp_state = mptcp_state_alloc();
        ph_state->mp_state = mp_state;
        mptcp_state_set_running(mp_state, 1);
        mptcp_state_set_event_cb(mp_state, policy_handler_mptcp_cb, ph_state);

        pthread_create(&mptcp_thread, 0, mptcp_control_start, mp_state);
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
