#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <arpa/inet.h>

#include <libmnl/libmnl.h>
#include <linux/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>

#include "resource_manager.h"

#include "list.h"
#include "debug.h"
#include "link_monitor.h"
#include "lmnl_interface.h"
#include "util.h"
#include "path_metrics/path_metric_interface.h"

#define PROBE_FREQUENCY 300

#define MPDD_RESOURCE_INTERFACE_PATH_DIRECT "/tmp/mpdd/direct/"
#define MPDD_RESOURCE_INTERFACE_PATH_INDIRECT "/tmp/mpdd/indirect/"

struct network_resource
{
    uint8_t table;
    uint8_t direct;
    int idx;
    uint32_t address;
    uint32_t gateway;
    uint32_t prio;
    uint32_t family;
    uint32_t link_type;
    char ifname[IFNAMSIZ];
    uint32_t multipath;
    uint32_t available;
    resource_availability_cb_t availability_cb;
    void *availability_data;
    struct path_stats *state;
    pthread_t resource_thread;
    pthread_mutex_t resource_lock;
    int thread_running;
};

int network_resource_complete(struct network_resource *nr);
pthread_t network_resource_get_thread(struct network_resource *nr);
void * network_resource_thread_start(void *data);
void network_resource_thread_stop(struct network_resource *nr);
void network_resource_set_running(struct network_resource *nr, int running);
int lookup_link_type(struct network_resource *nr);

struct resource_thread_params {
    struct network_resource *nr;
    char * endpoint;
    uint32_t probe_frequency;
};

int network_resource_set_link_profile(struct network_resource *nr, List *profiles)
{
    Litem *item = (Litem*)0;
    list_for_each(item, profiles){
        struct physical_link *pl;
        pl = (struct physical_link *)item->data;
        if(!strcmp(physical_link_get_ifname(pl), nr->ifname)){
            nr->multipath = physical_link_get_multipath(pl);
            nr->link_type = physical_link_get_type(pl);
            return 0;
        }
    }
    return -1;
}

void network_resource_list_put_cb(List *l, Litem *item, void *data)
{
    struct network_resource_callback_data *cb_data;
    struct resource_thread_params *rtp;
    struct network_resource *nr = (struct network_resource*)0;
    char *measurement_endpoint = (char*)0;

    nr = (struct network_resource*)item->data;
    cb_data = (struct network_resource_callback_data *)data;
    measurement_endpoint = (char*)cb_data->measurement_endpoint;

    network_resource_set_link_profile(nr, cb_data->link_profiles);

    network_resource_set_running(nr, 1);

    rtp = malloc(sizeof(struct resource_thread_params));

    rtp->nr = nr;
    rtp->endpoint = measurement_endpoint;
    rtp->probe_frequency = PROBE_FREQUENCY;

    pthread_create(&(nr->resource_thread), NULL,
               (void*)&network_resource_thread_start, (void*)rtp);
}

void network_resource_list_rem_cb(List *l, Litem *item, void *data)
{
    int
    blocking = 0;
    struct network_resource *nr = (struct network_resource*)0;

    blocking = (int*)data;

    nr = (struct network_resource*)item->data;

    network_resource_thread_stop(nr);

    if(blocking){
        pthread_join(network_resource_get_thread(nr), NULL);
    }
}

void network_resource_thread_stop(struct network_resource *nr)
{
    network_resource_set_running(nr, 0);
}


void * network_resource_thread_start(void *data)
{
    struct resource_thread_params *rtp;
    struct network_resource *nr = (struct network_resource *)0;
    char *endpoint = (char*)0;
    char *local = (char*)0;
    uint32_t probe_frequency;
    uint32_t host_addr = 0;

    rtp = (struct resource_thread_params *)data;
    nr = rtp->nr;
    endpoint = rtp->endpoint;
    probe_frequency = rtp->probe_frequency;

    host_addr = ntohl(nr->address);

    local = malloc(sizeof(char)*32);
    memset(local, 0, sizeof(char)*32);
    sprintf(local, "%d.%d.%d.%d",
        IPCOMP(host_addr, 0),
        IPCOMP(host_addr, 1),
        IPCOMP(host_addr, 2),
        IPCOMP(host_addr, 3));

    print_debug("Starting Resource Measurement (%s) from %s to %s\n", nr->ifname, local, endpoint);

    while(network_resource_is_running(nr)){
        if(network_resource_complete(nr)){
            struct path_stats *ps = metric_clone(nr->state);
            metric_update(nr->state, endpoint, local);
            if(metric_cmp(ps, nr->state)){
                /*Fire Metric Change Callback*/
                if(nr->availability_cb){
                    nr->availability_cb(nr, nr->availability_data);
                }
            }
            print_path_stats(nr->state, nr->ifname);
        } else {
            print_verb("Network Resource Struct not yet fully populated\n");
        }
        sleep(probe_frequency);
    }

    free(local);
    free(rtp);

    return (void*)0;
}

int network_resource_complete(struct network_resource *nr)
{
    if(!nr->address){
        return 0;
    }
    if(!strlen(nr->ifname)){
        return 0;
    }
    return 1;
}


int network_resource_cmp(struct network_resource *nr1, struct network_resource *nr2)
{
    if(nr1->address != nr2->address){
        return 1;
    }

    if(nr1->gateway != nr2->gateway){
        return 1;
    }

    if(nr1->idx != nr2->idx){
        return 1;
    }

    return 0;
}

int network_resource_add_to_list(List *nr_list, struct mnl_route *rt)
{
    Litem *item;
    struct network_resource *net_res;

    if(!nr_list){
        print_verb("list is null\n");

        return -1;
    }

    net_res = mnl_route_to_resource(rt);


    net_res->table = read_mpdd_file_table(net_res->address, &(net_res->direct));

    print_verb("Set Table: %d\n", net_res->table);
    print_verb("Set Resource Loc: %d\n", net_res->direct);

    item = malloc(sizeof(Litem));
    if(!item){
        print_verb("malloc failed\n");

        return -1;
    }
    item->data = net_res;
    list_put(nr_list, item);
    print_verb("network_resource added to list\n");
    return 0;
}

int network_resource_delete_from_list(List *nr_list, struct mnl_route *rt)
{
    int idx = 0;
    Litem *item;
    struct network_resource *net_res;
    net_res = mnl_route_to_resource(rt);

    list_for_each(item, nr_list){
        struct network_resource *nr = item->data;
        if(!network_resource_cmp(net_res, nr)){
            list_remove(nr_list, idx);
            network_resource_free(nr);
            free(item);
            print_verb("Deleted network_resource from list\n");
            return 0;
        }
        idx++;
    }
    print_verb("network_resource not in list\n");

    return -1;
}

struct network_resource * network_resource_alloc(void)
{
    struct network_resource *res;

    if(!(res = malloc(sizeof(struct network_resource)))) {
        print_error("malloc failed\n");
        return (struct network_resource *)0;
    }

    if (pthread_mutex_init(&(res->resource_lock), NULL) != 0) {
        print_error("resource_lock mutex_init failed\n");
        return (struct network_resource *)0;
    }

    res->table = 0;
    res->idx = 0;
    res->address = 0;
    res->gateway = 0;
    res->prio = 0;
    res->family = 0;
    res->link_type = 0;

    res->state = metric_alloc();

    metric_set_bandwidth(res->state, 0);
    metric_set_delay(res->state, -1);
    metric_set_jitter(res->state, -1);
    metric_set_loss(res->state, -1);

    res->thread_running = 0;

    memset(res->ifname, 0, IFNAMSIZ);

    return res;
}

void network_resource_set_availability_cb(
  struct network_resource *nr,
  resource_availability_cb_t cb,
  void* data)
{
    nr->availability_cb = cb;
    nr->availability_data = data;
}

void network_resource_free(struct network_resource *nr)
{
    free(nr);
}

int network_resource_is_running(struct network_resource *nr)
{
    return nr->thread_running;
}

void network_resource_set_running(struct network_resource *nr, int running)
{
    nr->thread_running = running;
}

pthread_t network_resource_get_thread(struct network_resource *nr)
{
    return nr->resource_thread;
}

char *network_resource_get_ifname(struct network_resource *nr)
{
    return nr->ifname;
}

int network_resource_get_table(struct network_resource *nr)
{
    return nr->table;
}

int network_resource_get_index(struct network_resource *nr)
{
    return nr->idx;
}

int network_resource_get_address(struct network_resource *nr)
{
    return nr->address;
}

int network_resource_get_gateway(struct network_resource *nr)
{
    return nr->gateway;
}

int network_resource_get_prio(struct network_resource *nr)
{
    return nr->prio;
}

int network_resource_get_family(struct network_resource *nr)
{
    return nr->family;
}

int network_resource_get_link_type(struct network_resource *nr)
{
    return nr->link_type;
}

struct path_stats * network_resource_get_state(struct network_resource *nr)
{
    return nr->state;
}

int lookup_link_type(struct network_resource *nr)
{
    return 0;
}

struct addr_cb_data {
    uint32_t addr;
    uint32_t prefix;
    uint32_t route;
};

int lookup_addr_cb(const struct nlmsghdr *nlh, void *data)
{
    struct ifaddrmsg *ifa = mnl_nlmsg_get_payload(nlh);
    struct nlattr *attr;
    struct addr_cb_data *cb_data;
    void *addr_ptr = (void*)0;
    uint32_t prefix = 0;

    prefix = ntohl(0xffffffff ^ ((1 << (32 - ifa->ifa_prefixlen)) - 1));

    cb_data = (struct addr_cb_data*)data;

    mnl_attr_for_each(attr, nlh, sizeof(*ifa)) {
        int type = mnl_attr_get_type(attr);

        if (mnl_attr_type_valid(attr, IFLA_MAX) < 0){
            continue;
        }

        switch(type) {
            case IFA_ADDRESS:
                if (mnl_attr_validate(attr, MNL_TYPE_BINARY) < 0) {
        			print_error("mnl_attr_validate\n");
        			return MNL_CB_ERROR;
        		}

                addr_ptr = mnl_attr_get_payload(attr);

                if((*(uint32_t*)addr_ptr & prefix) == (cb_data->route & prefix)){
                    cb_data->addr = *(uint32_t*)addr_ptr;
                    cb_data->prefix = prefix;
                }

        		break;
        }
    }
    return MNL_CB_OK;
}

uint32_t lookup_address(struct network_resource *nr)
{
    struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtgenmsg *rt;
	int ret;
	unsigned int seq, portid;
    struct addr_cb_data cb_data;

    cb_data.route = nr->gateway;
    cb_data.addr = 0;
    cb_data.prefix = 0;

    nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_GETADDR;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);
	rt = mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
	rt->rtgen_family = AF_INET;

	nl = mnl_socket_open(NETLINK_ROUTE);
	if (nl == NULL) {
		print_error("mnl_socket_open\n");
        return 0;
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		print_error("mnl_socket_bind\n");
        return 0;
	}
	portid = mnl_socket_get_portid(nl);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		print_error("mnl_socket_send\n");
        return 0;
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, lookup_addr_cb, &cb_data);
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		print_error("error\n");
        return 0;
	}

	mnl_socket_close(nl);

    return cb_data.addr;
}

struct name_cb_data {
    char *name;
    int idx;
};

int lookup_name_cb(const struct nlmsghdr *nlh, void *data)
{
	struct ifinfomsg *ifm = mnl_nlmsg_get_payload(nlh);
	struct nlattr *attr;
    struct name_cb_data *cb_data;

    cb_data = (struct name_cb_data*)data;
    /*
	printf("index=%d type=%d flags=%d family=%d ",
		ifm->ifi_index, ifm->ifi_type,
		ifm->ifi_flags, ifm->ifi_family);
    */

    if(ifm->ifi_index != cb_data->idx){
        return MNL_CB_OK;
    }

	mnl_attr_for_each(attr, nlh, sizeof(*ifm)) {
		int type = mnl_attr_get_type(attr);

		if (mnl_attr_type_valid(attr, IFLA_MAX) < 0)
			continue;

		switch(type) {
    		case IFLA_IFNAME:
    			if (mnl_attr_validate(attr, MNL_TYPE_STRING) < 0) {
    				print_error("mnl_attr_validate\n");
    				return MNL_CB_ERROR;
    			}
                print_verb("Name Found: %s\n", mnl_attr_get_str(attr));
    			strcpy(cb_data->name, mnl_attr_get_str(attr));
                trimwhitespace(cb_data->name);
    			break;
		}
	}
	return MNL_CB_OK;
}

int lookup_name(struct network_resource *nr, char *name)
{
	struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtgenmsg *rt;
	int ret;
	unsigned int seq, portid;

    struct name_cb_data cb_data;

    cb_data.name = name;
    memset(cb_data.name, 0, IFNAMSIZ);
    cb_data.idx = nr->idx;

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= RTM_GETLINK;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlh->nlmsg_seq = seq = time(NULL);
	rt = mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtgenmsg));
	rt->rtgen_family = AF_PACKET;

	nl = mnl_socket_open(NETLINK_ROUTE);
	if (nl == NULL) {
		print_error("mnl_socket_open\n");
        return -1;
	}

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
		print_error("mnl_socket_bind\n");
        return -1;
	}
	portid = mnl_socket_get_portid(nl);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
		print_error("mnl_socket_send\n");
        return -1;
	}

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	while (ret > 0) {
		ret = mnl_cb_run(buf, ret, seq, portid, lookup_name_cb, &cb_data);
		if (ret <= MNL_CB_STOP)
			break;
		ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	}
	if (ret == -1) {
		print_error("error\n");
        return -1;
	}

	mnl_socket_close(nl);

	return 0;
}

struct network_resource *mnl_route_to_resource(struct mnl_route *route)
{
    struct network_resource *res = (struct network_resource *)0;

    if(!route){
        return (struct network_resource *)0;
    }

    if(!(res = network_resource_alloc())){
        return (struct network_resource *)0;
    }

    res->table = route->table;
    res->idx = route->idx;
    res->gateway = route->gateway;
    res->prio = route->prio;
    res->family = route->family;
    res->link_type = lookup_link_type(res);
    res->address = lookup_address(res);
    lookup_name(res, res->ifname);
    return res;
}


int read_mpdd_file_table(uint32_t ip_address, uint8_t *resource_type)
{
    struct stat st = {0};
    char path[512];
    int table = 0;

    memset(path, 0, 512);
    sprintf(path, "%s%"PRIu32"_table", MPDD_RESOURCE_INTERFACE_PATH_DIRECT, ip_address);
    if (!stat(path, &st)) {
        char *data = (char*)0;
        *resource_type = DIRECT_RESOURCE;
        data = load_file(path);
        if(!data){
            return 0;
        }
        table = atoi(data);
        free(data);
        return table;
    }

    memset(path, 0, 512);
    sprintf(path, "%s%"PRIu32"_table", MPDD_RESOURCE_INTERFACE_PATH_INDIRECT, ip_address);
    if (!stat(path, &st)) {
        char *data = (char*)0;
        *resource_type = INDIRECT_RESOURCE;
        data = load_file(path);
        if(!data){
            return 0;
        }
        table = atoi(data);
        free(data);
        return table;
    }

    return table;
}
