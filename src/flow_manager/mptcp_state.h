#ifndef __MPTCP_STATE__
#define __MPTCP_STATE__

#include <stdint.h>

#include "../list.h"

struct mptcp_state;
struct connection;

struct connection {
    uint32_t token;
    uint32_t saddr;
    uint32_t daddr;
    uint16_t sport;
    uint16_t dport;
    uint8_t transport;
    uint8_t multipath;
    List *subflows;
};

struct subflow{
    uint32_t saddr;
};

typedef void * (*mptcp_control_cb_t)(void *context);

int mptcp_check_local_capability(void);

struct connection * mptcp_state_pop_connection_by_token(struct mptcp_state *mp_state, uint32_t token);
void mptcp_state_put_connection(struct mptcp_state *state, struct connection *c);
struct connection *mptcp_find_connection(List *connections, uint32_t saddr, uint32_t daddr, uint32_t dport);

int mptcp_state_lock(struct mptcp_state *state);
int mptcp_state_unlock(struct mptcp_state *state);

/*Setters*/
void mptcp_state_set_comms(struct mptcp_state *state, void *comm);
void mptcp_state_set_running(struct mptcp_state *state, int running);

/*Getters*/
List * mptcp_state_get_connections(struct mptcp_state *state);
void * mptcp_state_get_comms(struct mptcp_state *state);
int mptcp_state_get_running(struct mptcp_state *state);

/*Memory*/
struct mptcp_state * mptcp_state_alloc(void);
void mptcp_state_free(struct mptcp_state *state);

#endif
