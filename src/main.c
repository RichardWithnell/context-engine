#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "policy.h"
#include "queue.h"
#include "list.h"
#include "link_monitor.h"
#include "link_manager_interface.h"
#include "debug.h"
#include "resource_manager.h"
#include "application_rules.h"

#include "conditions/condition.h"
#include "context_library.h"

#include "flow_manager/policy_handler.h"

#define CONTEXT_MANAGER_PID "/var/run/context_manager.pid"

#define LINK_CONFIG "./config/link.cfg\0"
#define POLICY_CONFIG "./config/policy.cfg\0"
#define APPLICATION_CONFIG "./config/application.cfg\0"

#define MEASUREMENT_ENDPOINT "zoostorm-lab.noip.me\0"

int running = 1;
sem_t update_barrier;

char *libs[] = {"src/conditions/libbattery_policy.so",
                    "src/conditions/libbandwidth_policy.so",
                    "src/conditions/liblocation_policy.so",
                    "src/conditions/libwifi_policy.so",
                    0};

struct condition_cb_data
{
    List *netres_list;
    List *application_specs;
    List *iptables_rules;
};

void sig_handler(int signum)
{
    if (signum == SIGINT) {
        running = 0;
    }
    sem_post(&update_barrier);
}

int create_pid(void)
{
    /*Write PID file*/
    remove(CONTEXT_MANAGER_PID);

    int pid_fd = open(CONTEXT_MANAGER_PID, O_RDWR | O_CREAT | O_TRUNC, 666);
    if(!pid_fd) {
        print_error("Could not open pid file.\n");
        return -1;
    } else {
        char pid_buff[128];
        snprintf(pid_buff, 128, "%ld\n", (long)getpid());

        if (write(pid_fd, pid_buff, strlen(pid_buff)) != strlen(pid_buff)) {
            print_error("Could not write to pid file.\n");
            return -1;
        }
        close(pid_fd);
    }

    return 0;
}

void condition_cb(struct condition *c, void *data)
{
    struct policy_handler_state *ph_state = (struct policy_handler_state*)0;
    struct policy_definition *pd = (struct policy_definition*)0;
    struct condition *condition = (struct condition*)0;
    struct action *action = (struct action*)0;
    List *conditions = (List*)0;
    List *actions = (List*)0;
    List *netres_list = (List*)0;
    List *application_specs = (List*)0;
    List *iptables_rules = (List*)0;
    Litem *item = (Litem*)0;

    ph_state = (struct policy_handler_state*) data;

    netres_list = policy_handler_state_get_resources(ph_state);
    iptables_rules = policy_handler_state_get_rules(ph_state);
    application_specs = policy_handler_state_get_specs(ph_state);

    pd = condition_get_parent(c);

    if(!pd){
        print_error("Conditions parent is null\n");
        return;
    }

    conditions = policy_definition_get_conditions(pd);
    if (!conditions){
        print_error("Conditions list is null\n");
        return;
    }

    actions = policy_definition_get_actions(pd);
    if (!actions){
        print_error("actions list is null\n");
        return;
    }

    list_for_each(item, conditions){
        condition = (struct condition*)item->data;
        if(!condition){
            print_error("Condition is null\n");
            return;
        }
        if(!condition_get_met(condition)){
            print_verb("Condition [%s] not met yet, wait\n",
                condition_get_key(condition));
            return;
        }
    }
    print_verb("All Conditions Met\n");
    /*All conditions Met*/

    print_verb("Performing %d actions\n", list_size(actions));
    list_for_each(item, actions) {
        int act_mode = 0;
        action = (struct action*)item->data;
        act_mode = action_get_mode(action);
        if(!action){
            print_error("Action is null: continue\n");
            continue;
        }

        action_print(action);

        if(act_mode == ACTION_MODE_HARD){
            print_verb("Performing Hard Action\n");
            char *lname = action_get_link_name(action);
            switch(action_get_action(action)) {
                case ACTION_DISABLE:
                    async_link_manager_down(lname);
                case ACTION_ENABLE:
                    async_link_manager_up(lname);
            }
        } else if (act_mode == ACTION_MODE_SOFT) {
            Litem *net_res = (Litem*)0;
            int update_flag = 0;
            print_verb("Performing Soft Action\n");

            print_verb("NetRes List Size: %d\n", list_size(netres_list));

            list_for_each(net_res, netres_list){
                struct network_resource *res = (struct network_resource*)0;
                res = (struct network_resource*)net_res->data;

                print_verb("Looking at resource: %s\n",
                    ip_to_str(htonl(network_resource_get_address(res))));

                print_verb("Compare: %s and %s\n",
                  network_resource_get_ifname(res),
                  action_get_link_name(action));

                if(!strcasecmp(network_resource_get_ifname(res),
                            action_get_link_name(action)))
                {
                    uint8_t act = action_get_action(action);
                    if(act == ACTION_ENABLE){
                        if(network_resource_get_available(res)
                          == RESOURCE_AVAILABLE)
                        {
                            print_verb("Resource Already in available state, "
                                        "do nothng\n");
                            continue;
                        } else {
                            print_verb("Action: Enable %s\n",
                                action_get_link_name(action));
                            network_resource_set_available(res,
                                RESOURCE_AVAILABLE);
                            /*Add Subflows*/
                            policy_handler_add_route_cb(res, ph_state);
                            update_flag = 1;
                        }
                    } else if(act == ACTION_DISABLE){
                        if(network_resource_get_available(res)
                          == RESOURCE_UNAVAILABLE)
                        {
                            print_verb("Resource Already in unavailable state, "
                                        "do nothng\n");
                            continue;
                        } else {
                            print_verb("Action: Disable %s\n",
                                action_get_link_name(action));
                            network_resource_set_available(res,
                                RESOURCE_UNAVAILABLE);

                            /*Remove Subflows*/
                            policy_handler_del_route_cb(res, ph_state);
                            update_flag = 1;
                        }
                    } else {
                        print_error("Unhandled action\n");
                    }
                }
            }

            if(update_flag){
                /*Recalculate Routes*/
                route_selector(netres_list,
                    application_specs,
                    iptables_rules);
            }
            /*Else no change actually occured*/


        } else {
            print_error("Unknown action mode: %d\n", act_mode);
        }
    }

    return;
}

void resource_down(struct network_resource *nr, struct policy_handler_state *ph)
{
    policy_handler_route_change_cb_t cb;
    print_debug("Resource Availability - Down: %s\n",
      network_resource_get_ifname(nr));

    cb = policy_handler_state_get_route_cb(ph);
    cb(nr, LINK_EVENT_DOWN, ph);

    return (void)0;
}

void resource_up(struct network_resource *nr, struct policy_handler_state *ph)
{
    policy_handler_route_change_cb_t cb;

    print_debug("Resource Availability - Up: %s\n",
      network_resource_get_ifname(nr));

    cb = policy_handler_state_get_route_cb(ph);

    cb(nr, LINK_EVENT_UP, ph);

    return (void)0;
}

void * metric_change_cb(struct network_resource *resource, void *data)
{
    struct condition_cb_data *ccbd;

    List *iptables_rules = (List*)0;
    List *application_specs = (List*)0;
    List *network_resources = (List*)0;

    ccbd = (struct condition_cb_data*)data;

    network_resources = ccbd->netres_list;
    application_specs = ccbd->application_specs;
    iptables_rules = ccbd->iptables_rules;

    route_selector(network_resources, application_specs, iptables_rules);

    print_debug("Metric change callback\n");

    return (void*)0;
}

int main(void)
{
    struct network_resource_callback_data nr_cb_data;
    struct policy_handler_state * ph_state;
    /*Define variables for libnl updates*/
    struct cache_monitor mon_data;
    pthread_t monitor_thread;
    struct condition_cb_data ccbd;
    pthread_mutex_t update_lock;
    Queue update_queue;

    char *measurement_endpoint = (char*)0;

    List *link_profiles = (List*)0;
    List *context_libs = (List*)0;
    List *policy_list = (List*)0;
    List *application_specs = (List*)0;
    List *netres_list = (List*)0;

    context_libs = malloc(sizeof(List));
    netres_list = malloc(sizeof(List));

    list_init(netres_list);
    list_init(context_libs);

    print_debug("Measurement Endpoint Len: %zd\n",
        strlen(MEASUREMENT_ENDPOINT));

    measurement_endpoint =
        malloc(sizeof(char)*(strlen(MEASUREMENT_ENDPOINT)+1));

    strcpy(measurement_endpoint, MEASUREMENT_ENDPOINT);
    measurement_endpoint[strlen(MEASUREMENT_ENDPOINT)] = 0;
    print_debug("Measurement Endpoint Set: %s\n", measurement_endpoint);

    link_profiles = load_link_profiles(LINK_CONFIG);
    if(!link_profiles) {
        print_error("Failed to load policy file: %s\n", LINK_CONFIG);
        return -1;
    }

    nr_cb_data.measurement_endpoint = measurement_endpoint;
    nr_cb_data.link_profiles = link_profiles;

    list_rem_register_cb(netres_list,
        (list_cb_t )network_resource_list_rem_cb, 0);

    list_put_register_cb(netres_list,
        (list_cb_t )network_resource_list_put_cb,
        (void*)&nr_cb_data);

    load_context_libs(context_libs, libs);
    policy_list = load_policy_file(POLICY_CONFIG, context_libs);
    if(!policy_list){
        print_error("Failed to load policy file: %s\n", POLICY_CONFIG);
        return -1;
    }

    application_specs = load_application_specs(APPLICATION_CONFIG);
    if(!application_specs){
        print_error("Failed to load application config file: %s\n",
            APPLICATION_CONFIG);
        return -1;
    }

    /*
    if(create_pid()){
        return -1;
    }
    */

    /*Register signal handler*/
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        print_error("Failed to register signal handler\n");
        return -1;
    }

    if(queue_init(&update_queue)) {
        printf("Queue init failed\n");
        return -1;
    }

    if (pthread_mutex_init(&update_lock, NULL) != 0) {
        print_error("Update mutex init failed\n");
        return -1;
    }

    if (sem_init(&update_barrier, 0, 0) != 0) {
        print_error("Update barrier init failed\n");
        return -1;
    }

    ph_state = policy_handler_init(netres_list, application_specs);

    mon_data.queue = &update_queue;
    mon_data.lock = &update_lock;
    mon_data.barrier = &update_barrier;

    pthread_create(&monitor_thread, 0,
                   (void*)&init_monitor, (void*)&mon_data);

    ccbd.netres_list = netres_list;
    ccbd.application_specs = application_specs;
    ccbd.iptables_rules = policy_handler_state_get_rules(ph_state);

    context_library_start(context_libs, condition_cb, ph_state);

    while(running) {
        print_verb("Waiting on barrier\n");
        sem_wait(&update_barrier);
        if(!running) {
            print_debug("Stopping\n");
            break;
        }
        print_verb("Recieved update\n");
        pthread_mutex_lock(&update_lock);
        Qitem* qitem = queue_get(&update_queue);
        pthread_mutex_unlock(&update_lock);

        if(qitem) {
            struct update_obj* u = qitem->data;
            if(!u){
                print_error("Update object is null\n");
                continue;
            }
            if(u->type == UPDATE_ROUTE) {
                struct network_resource *nr = (struct network_resource *)0;
                if(u->action == ADD_RT) {
                    nr = network_resource_add_to_list(netres_list,
                            (struct mnl_route*)u->update,
                            metric_change_cb,
                            &ccbd);

                    if(nr){
                        resource_up(nr, ph_state);
                    } else {
                        print_error("Failed to create resource\n");
                    }

                } else if(u->action == DEL_RT) {
                    nr = network_resource_delete_from_list(netres_list,
                            (struct mnl_route*)u->update);
                    if(nr){
                        resource_down(nr, ph_state);
                        network_resource_free(nr);
                    } else {
                        print_error("Failed to pop a non null resource\n");
                    }
                }
            }
        }

        free(qitem);
    }
    printf("Done.\n");
    return 0;
}
