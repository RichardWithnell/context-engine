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
    uint32_t loc_id;
    List *subflows;
};

struct subflow {
    uint32_t saddr;
    uint32_t loc_id;
};

typedef enum {
    MPTCP_NEW_FLOW = 0x01,
    MPTCP_REM_FLOW = 0x02,
    MPTCP_NEW_ADDR = 0x03,
    MPTCP_REM_ADDR = 0x04,
    MPTCP_NEW_CONN = 0x03,
    MPTCP_REM_CONN = 0x04,
} mptcp_event_codes;

typedef void (*connection_mod_cb_t)(struct mptcp_state *state, struct connection *connection, void* data);
typedef void * (*mptcp_control_cb_t)(struct mptcp_state *state, struct connection *connection, uint8_t mode, void* data);

int mptcp_check_local_capability(void);

void mptcp_connection_add_subflow(struct connection *conn, uint32_t address, uint32_t loc_id);

void mptcp_for_each_connection(struct mptcp_state *mp_state, connection_mod_cb_t cb, void *data);

struct connection * mptcp_state_pop_connection_by_token(struct mptcp_state *mp_state, uint32_t token);
void mptcp_state_put_connection(struct mptcp_state *state, struct connection *c);
struct connection *mptcp_find_connection(List *connections, uint32_t saddr, uint32_t daddr, uint32_t dport);

int mptcp_state_lock(struct mptcp_state *state);
int mptcp_state_unlock(struct mptcp_state *state);

/*Setters*/
void mptcp_state_set_comms(struct mptcp_state *state, void *comm);
void mptcp_state_set_running(struct mptcp_state *state, int running);
void mptcp_state_set_event_cb(struct mptcp_state *state, mptcp_control_cb_t cb, void *data);
void mptcp_state_set_cb_data(struct mptcp_state *state, void* data);

/*Getters*/
List * mptcp_state_get_connections(struct mptcp_state *state);
void * mptcp_state_get_comms(struct mptcp_state *state);
int mptcp_state_get_running(struct mptcp_state *state);
mptcp_control_cb_t mptcp_state_get_event_cb(struct mptcp_state *state);
void * mptcp_state_get_cb_data(struct mptcp_state *state);

/*Memory*/
struct mptcp_state * mptcp_state_alloc(void);
void mptcp_state_free(struct mptcp_state *state);

#endif
