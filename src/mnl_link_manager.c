#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#include <libmnl/libmnl.h>
#include <linux/if.h>
#include <linux/if_link.h>
#include <linux/rtnetlink.h>

#include "mnl_link_manager.h"
#include "debug.h"

enum {
    LINK_UNSPEC,
    LINK_UP,
    LINK_DOWN
};


int mnl_link_manager_mod(char *ifname, int mode)
{
    struct mnl_socket *nl;
    char buf[MNL_SOCKET_BUFFER_SIZE];
    struct nlmsghdr *nlh;
    struct ifinfomsg *ifm;
    int ret;
    unsigned int seq, portid, change = 0, flags = 0;

    if(mode == LINK_UP){
        change |= IFF_UP;
        flags |= IFF_UP;
    } else if(mode == LINK_DOWN){
        change |= IFF_UP;
        flags &= ~IFF_UP;
    } else {
        print_error("Unknown link mod mode\n");
        return -1;
    }

    nlh = mnl_nlmsg_put_header(buf);
    nlh->nlmsg_type	= RTM_NEWLINK;
    nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
    nlh->nlmsg_seq = seq = time(0);
    ifm = mnl_nlmsg_put_extra_header(nlh, sizeof(*ifm));
    ifm->ifi_family = AF_UNSPEC;
    ifm->ifi_change = change;
    ifm->ifi_flags = flags;

    mnl_attr_put_str(nlh, IFLA_IFNAME, ifname);

    nl = mnl_socket_open(NETLINK_ROUTE);
    if (!nl) {
        print_error("mnl_socket_open\n");
        return -1;
    }

    if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0) {
        print_error("mnl_socket_bind\n");
        return -1;
    }
    portid = mnl_socket_get_portid(nl);

    mnl_nlmsg_fprintf(stdout, nlh, nlh->nlmsg_len,
    sizeof(struct ifinfomsg));

    if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0) {
        print_error("mnl_socket_send\n");
        return -1;
    }

    ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
    if (ret == -1) {
        print_error("read\n");
        return -1;
    }

    ret = mnl_cb_run(buf, ret, seq, portid, 0, 0);
    if (ret == -1){
        print_error("callback\n");
        perror("Callback\n");
        return -1;
    }

    mnl_socket_close(nl);

    return 0;
}

int mnl_link_manager_down(char *ifname)
{
    return mnl_link_manager_mod(ifname, LINK_DOWN);
}

int mnl_link_manager_up(char *ifname)
{
    return mnl_link_manager_mod(ifname, LINK_UP);
}
