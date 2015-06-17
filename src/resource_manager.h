#ifndef CONTEXT_RESOURCE_MANAGER
#define CONTEXT_RESOURCE_MANAGER
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libmnl/libmnl.h>
#include <linux/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>

#include "debug.h"
#include "link_monitor.h"
#include "lmnl_interface.h"
#include "util.h"

struct device_state;
struct network_resource;
struct physical_link;

struct network_resource_callback_data {
    char *measurement_endpoint;
    List *link_profiles;
};

enum {
    UNKNOWN_RESOURCE,
    DIRECT_RESOURCE,
    INDIRECT_RESOURCE,
    __RESOURCE_TYPE_MAX
};

enum {
    LINK_EVENT_UNSPEC,
    LINK_EVENT_UP,
    LINK_EVENT_DOWN
};

enum {
    LINK_TYPE_UNSPEC,
    LINK_TYPE_CELLULAR,
    LINK_TYPE_WIFI,
    LINK_TYPE_ETHERNET,
    LINK_TYPE_WIMAX,
    LINK_TYPE_SATELLITE,
    LINK_TYPE_BLUETOOTH,
    __LINK_TYPE_MAX
};

enum {
    RESOURCE_AVAILABILITY_UNPSEC,
    RESOURCE_AVAILABLE,
    RESOURCE_UNAVAILABLE,
    RESOURCE_BACKUP,
    __RESOURCE_AVAILABILITY_MAX
};

typedef void * (*resource_availability_cb_t)(struct network_resource *nr, void *data);


int read_mpdd_file_table(uint32_t ip_address, uint8_t *resource_type);

struct network_resource *mnl_route_to_resource(struct mnl_route *route);

void network_resource_list_put_cb(List *l, Litem *item, void *data);
void network_resource_list_rem_cb(List *l, Litem *item, void *data);

struct network_resource * network_resource_add_to_list(List *nr_list, struct mnl_route *rt, resource_availability_cb_t cb, void *data);
struct network_resource * network_resource_delete_from_list(List *nr_list, struct mnl_route *rt);

struct network_resource * network_resource_alloc(void);
void network_resource_free(struct network_resource *nr);

void network_resource_set_multipath(struct network_resource *nr, uint32_t mp);
void network_resource_set_available(struct network_resource *nr, uint32_t available);

int network_resource_get_table(struct network_resource *nr);
char *network_resource_get_ifname(struct network_resource *nr);
int network_resource_get_index(struct network_resource *nr);
int network_resource_get_address(struct network_resource *nr);
int network_resource_get_gateway(struct network_resource *nr);
int network_resource_get_prio(struct network_resource *nr);
int network_resource_get_family(struct network_resource *nr);
int network_resource_get_link_type(struct network_resource *nr);
uint32_t network_resource_get_multipath(struct network_resource *nr);
uint32_t network_resource_get_available(struct network_resource *nr);
uint32_t network_resource_get_loc_id(struct network_resource *nr);

struct path_stats * network_resource_get_state(struct network_resource *nr);

int network_resource_is_running(struct network_resource *nr);

/*Link Loader Functions*/
List * load_link_profiles(char *config_file);
struct physical_link * physical_link_alloc(void);
void physical_link_free(struct physical_link *pl);
uint8_t physical_link_get_type(struct physical_link *pl);
char * physical_link_get_ifname(struct physical_link *pl);
uint8_t physical_link_get_multipath(struct physical_link *pl);
void physical_link_set_type(struct physical_link *pl, uint8_t type);
void physical_link_set_ifname(struct physical_link *pl, char *ifname);
void physical_link_set_multipath(struct physical_link *pl, uint8_t multipath);

#endif
