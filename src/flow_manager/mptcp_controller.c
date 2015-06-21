#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <errno.h>
#include <error.h>
#include <math.h>
#include <pthread.h>
#include <fcntl.h>

#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/utils.h>
#include <netlink/errno.h>
#include <linux/genetlink.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <signal.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "mptcp_controller.h"
#include "../util.h"
#include "../debug.h"

/********************************************************
* GENL Setup Data
********************************************************/

#define CA_PM_GENL_VERSION 1
#define CA_PM_GROUP_NAME "CA_PM_GROUP"
#define CA_PM_FAMILY_NAME "CA_PM_FAMILY"
#define CA_PM_GENL_HDRLEN	0

typedef enum {
    CONTEXT_CMD_CREATE_SUBFLOW,
    CONTEXT_CMD_REMOVE_SUBFLOW,
    CONTEXT_CMD_MOD_SUBFLOW,
    __CONTEXT_RECV_CMD_MAX
} E_CONTEXT_RECV_CMD;

typedef enum {
    CONTEXT_CMD_REGISTER_SESSION,
    CONTEXT_CMD_REMOVE_SESSION,
    __CONTEXT_SEND_CMD_MAX
} E_CONTEXT_SEND_CMD;

typedef enum {
    CONTEXT_ATTR_UNSPEC,
    CONTEXT_DST_PORT,
    CONTEXT_SRC_ADDR,
    CONTEXT_DST_ADDR,
    CONTEXT_LOC_ID,
    CONTEXT_MPTCP_TOKEN,
    __CONTEXT_MAX
} E_CONTEXT_ATTRIB ;


struct context_genl_msghdr {
    uint32_t dest;		/* Destination address */
    uint16_t cmd;		/* Command */
    uint16_t reserved;		/* Unused */
};
/********************************************************
* GENL End
********************************************************/

#define TCP 6
#define UDP 17

struct nl_sock_data {
    struct nl_sock *sock;
    int family_id;
    int group_id;
};

struct event {
    uint8_t code;
};

struct event_flow {
    struct event super;
    uint32_t daddr;
    uint32_t saddr;
    uint16_t dport;
    uint16_t sport;
    uint32_t token;
};

static int new_mptcp_event(struct nl_msg *msg, void *arg);


int mod_subflow(
  struct mptcp_state *mp_state,
  uint32_t src_ip,
  uint32_t src_locid,
  struct connection *conn,
  int command)
{
    int rc = 0;
    struct nl_sock_data *nlsd;
    struct nl_sock *sock;
    struct nl_msg *msg;
    msg = nlmsg_alloc();

    nlsd = (struct nl_sock_data *)mptcp_state_get_comms(mp_state);

    sock = nlsd->sock;

    genlmsg_put(msg,
        0,
        0,
        nlsd->family_id,
        CA_PM_GENL_HDRLEN,
        0,
        command,
        CA_PM_GENL_VERSION);

    //print_debug("Subflow: %s -> %s:%d\n", src->name, ip_to_str(htonl(conn->daddr)), conn->dport);

    rc = nla_put_u32 (msg, CONTEXT_MPTCP_TOKEN, conn->token);
    if(rc < 0) print_error("NLA PUT ERROR CONTEXT_MPTCP_TOKEN\n");

    rc = nla_put_u32 (msg, CONTEXT_SRC_ADDR, src_ip);
    if(rc < 0) print_error("NLA PUT ERROR CONTEXT_SRC_ADDR\n");

    rc = nla_put_u32 (msg, CONTEXT_DST_ADDR, conn->daddr);
    if(rc < 0) print_error("NLA PUT ERROR CONTEXT_DST_ADDR\n");

    rc = nla_put_u16 (msg, CONTEXT_DST_PORT, conn->dport);
    if(rc < 0) print_error("NLA PUT ERROR CONTEXT_DST_PORT\n");

    rc = nla_put_u16 (msg, CONTEXT_LOC_ID, src_locid);
    if(rc < 0) print_error("NLA PUT ERROR CONTEXT_LOC_ID\n");

    nl_send_auto(sock, msg);
    nlmsg_free(msg);

    return 0;
}

int mptcp_create_subflow(
  struct mptcp_state *mp_state,
  uint32_t src_ip,
  uint16_t src_locid,
  struct connection *conn)
{
    print_debug("Create Subflow\n");
    return mod_subflow(mp_state, src_ip, src_locid, conn, CONTEXT_CMD_CREATE_SUBFLOW);
}

int mptcp_remove_subflow(
  struct mptcp_state *mp_state,
  uint32_t src_ip,
  uint16_t src_locid,
  struct connection *conn)
{
    print_debug("Remove Subflow\n");
    return mod_subflow(mp_state, src_ip, src_locid, conn, CONTEXT_CMD_REMOVE_SUBFLOW);
}

static int remove_connection(struct genlmsghdr *ghdr, struct mptcp_state *mp_state)
{
    mptcp_control_cb_t cb;
    struct nlattr *attrs = (struct nlattr *)0;
    struct nlattr *nla = (struct nlattr *)0;
    int attrlen = 0;
    int rem = 0;
    int token = 0;
    struct connection conn;

    attrlen = genlmsg_attrlen(ghdr, 0);
    attrs = genlmsg_attrdata(ghdr, 0);

    print_debug("\n");

    nla_for_each_attr(nla, attrs, attrlen, rem){
        uint32_t *data = nla_data(nla);

        if(nla->nla_type == CONTEXT_MPTCP_TOKEN){
            token = *data;
        }
    }

    conn.token = token;

    cb = mptcp_state_get_event_cb(mp_state);
    if(cb){
        cb(mp_state, &conn, MPTCP_REM_CONN, mptcp_state_get_cb_data(mp_state));
    }
    return 0;
}

static int add_connection(struct genlmsghdr *ghdr, struct mptcp_state *mp_state)
{
    mptcp_control_cb_t cb;
    struct nlattr *attrs;
    struct nlattr *nla;
    int attrlen;
    int rem;
    struct connection *new_conn;

    print_debug("\n");

    attrlen = genlmsg_attrlen(ghdr, 0);
    attrs = genlmsg_attrdata(ghdr, 0);

    new_conn = malloc(sizeof(struct connection));
    new_conn->subflows = malloc(sizeof(List));
    new_conn->token = 0;
    new_conn->daddr = 0;
    new_conn->dport = 0;
    new_conn->sport = 0;
    new_conn->saddr = 0;

    list_init(new_conn->subflows);

    nla_for_each_attr(nla, attrs, attrlen, rem){
        uint32_t *data = nla_data(nla);

        if(nla->nla_type == CONTEXT_SRC_ADDR){
            new_conn->saddr = *data;
        } else if(nla->nla_type == CONTEXT_DST_ADDR){
            new_conn->daddr = *data;
        } else if(nla->nla_type == CONTEXT_DST_PORT){
            new_conn->dport = *data;
        } else if(nla->nla_type == CONTEXT_MPTCP_TOKEN){
            new_conn->token = *data;
        }
    }
    print_debug("New Connection: %s:%d (%04x)\n", ip_to_str(htonl(new_conn->daddr)), new_conn->dport, new_conn->token);

    new_conn->transport = TCP;

    if(new_conn->token == 0 || new_conn->daddr == 0 || new_conn->dport == 0){
        return -1;
    }

    cb = mptcp_state_get_event_cb(mp_state);
    if(cb){
        cb(mp_state, new_conn, MPTCP_NEW_CONN, mptcp_state_get_cb_data(mp_state));
    }

    return 0;
}

static int new_mptcp_event(struct nl_msg *msg, void *arg)
{
    struct genlmsghdr *ghdr;
    struct mptcp_state *mp_state;

    print_verb("New mptcp netlink event\n");

    mp_state = (struct mptcp_state *)arg;

    ghdr = (struct genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));

    if(ghdr->cmd == CONTEXT_CMD_REMOVE_SESSION){
        print_debug("Remove session\n");
        return remove_connection(ghdr, mp_state);
    } else if(ghdr->cmd == CONTEXT_CMD_REGISTER_SESSION){
        print_debug("Register session\n");
        return add_connection(ghdr, mp_state);
    } else {
        print_error("unknown commans\n");
    }

    return -1;
}


void * mptcp_control_start(void *data)
{
    struct nl_sock_data *nlsd = (struct nl_sock_data*)0;
    struct mptcp_state *mp_state = (struct mptcp_state*)0;
    struct nl_sock *sock = (struct nl_sock*)0;
    int family_id = 0;
    int group_id = 0;
    int rc = 0;

    mp_state = (void*)data;
    nlsd = malloc(sizeof(struct nl_sock_data));

    if(!nlsd){
        print_error("Failed to allocate libnl data struct\n");
        return (void*)-1;
    }

    sock = nl_socket_alloc();
    if(!sock) {
        print_error("Failed to alloc socket\n");
        return (void*)-1;
    }


    nl_socket_disable_seq_check(sock);
    nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM, new_mptcp_event, mp_state);
    genl_connect(sock);

    family_id = genl_ctrl_resolve(sock, CA_PM_FAMILY_NAME);
    if(family_id < 0) {
        print_error("Could not resolve family: %s Err: %s\n", CA_PM_FAMILY_NAME, nl_geterror(family_id));
        return (void*)-1;
    }

    group_id = genl_ctrl_resolve_grp(sock, CA_PM_FAMILY_NAME, CA_PM_GROUP_NAME);
    if(group_id < 0) {
        print_error("Could not resolve group: %s Err: %s\n", CA_PM_GROUP_NAME, nl_geterror(group_id));
        return (void*)-1;
    }

    rc = nl_socket_add_membership(sock, group_id);
    if(rc < 0){
        print_error("Could not add membership: Err: %s\n", nl_geterror(rc));
        return (void*)-1;
    }

    nlsd->sock = sock;
    nlsd->family_id = family_id;
    nlsd->group_id = group_id;
    mptcp_state_set_comms(mp_state, nlsd);

    print_debug("Waiting for callbacks\n");
    while (mptcp_state_get_running(mp_state)) {
        nl_recvmsgs_default(sock);
    }
    print_debug("MPTCP Controller Done\n");

    return (void*)0;
}
