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
#include "debug.h"
#include "resource_manager.h"
#include "application_rules.h"

#include "conditions/condition.h"
#include "context_library.h"

#include "flow_manager/policy_handler.h"

#define CONTEXT_MANAGER_PID "/var/run/context_manager.pid"

#define MEASUREMENT_ENDPOINT "zoostorm-lab.noip.me\0"

int running = 1;
sem_t update_barrier;

char *libs[] = {"src/conditions/libbattery_policy.so",
                    "src/conditions/libbandwidth_policy.so",
                    "src/conditions/liblocation_policy.so",
                    0};

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
    print_debug("Condition Callback fired: %s\n", condition_get_key(c));
    return;
}

void * resource_availability_cb(struct network_resource *nr, void *data)
{
    print_debug("Resource Availability Changed: %s\n",
      network_resource_get_ifname(nr));
    return (void*)0;
}


int main(void)
{
    struct network_resource_callback_data nr_cb_data;

    /*Define variables for libnl updates*/
    struct cache_monitor mon_data;
    pthread_t monitor_thread;

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

    print_debug("Measurement Endpoint Len: %zd\n", strlen(MEASUREMENT_ENDPOINT));
    measurement_endpoint = malloc(sizeof(char)*(strlen(MEASUREMENT_ENDPOINT)+1));
    strcpy(measurement_endpoint, MEASUREMENT_ENDPOINT);
    measurement_endpoint[strlen(MEASUREMENT_ENDPOINT)] = 0;
    print_debug("Measurement Endpoint Set: %s\n", measurement_endpoint);

    link_profiles = load_link_profiles("./example/link.cfg\0");
    if(!link_profiles){
        print_error("Failed to load policy file: %s\n", "./example/link.cfg");
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
    policy_list = load_policy_file("./example/policy.cfg\0", context_libs);
    if(!policy_list){
        print_error("Failed to load policy file: %s\n", "./example/policy.cfg");
        return -1;
    }

    application_specs = load_application_specs("./example/application.cfg\0");
    if(!application_specs){
        print_error("Failed to load application config file: %s\n", "./example/application.cfg");
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

    policy_handler_init();

    mon_data.queue = &update_queue;
    mon_data.lock = &update_lock;
    mon_data.barrier = &update_barrier;

    pthread_create(&monitor_thread, NULL,
                   (void*)&init_monitor, (void*)&mon_data);


    context_library_start(context_libs, condition_cb);


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
                if(u->action == ADD_RT) {
                    network_resource_add_to_list(netres_list, (struct mnl_route*)u->update);
                } else if(u->action == DEL_RT) {
                    network_resource_delete_from_list(netres_list, (struct mnl_route*)u->update);
                }
            }
        }

        free(qitem);
    }
    printf("Done.\n");
    return 0;
}
