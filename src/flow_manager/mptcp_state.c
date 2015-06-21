#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#include "mptcp_state.h"
#include "../util.h"
#include "../debug.h"

struct mptcp_state {
    List *connections;
    List *mp_connections;
    List *sp_connections;
    List *migratable_connections;
    void *kernel_comms;
    int running;
    mptcp_control_cb_t event_cb;
    void *cb_data;
    pthread_mutex_t lock;
};

struct subflow * mptcp_connection_remove_subflow(struct connection *conn, uint32_t address)
{
    int idx = 0;
    Litem *item;

    list_for_each(item, conn->subflows){
        struct subflow *sf = (struct subflow*)item->data;
        if(sf->saddr == address){
            list_remove(conn->subflows, idx);
            free(item);
            print_verb("Deleted network_resource from list\n");
            return sf;
        }
        idx++;
    }
    return (struct subflow*)0;
}

void mptcp_connection_add_subflow(struct connection *conn, uint32_t address, uint32_t loc_id, void* data)
{
    struct subflow *s;
    Litem *item;

    s = malloc(sizeof(struct subflow));
    s->saddr = address;
    s->loc_id = loc_id;
    s->resource = data;
    item = malloc(sizeof(Litem));
    item->data = s;
    list_put(conn->subflows, item);
}

void * mptcp_state_get_cb_data(struct mptcp_state *state)
{
    return state->cb_data;
}

void mptcp_state_set_cb_data(struct mptcp_state *state, void* data)
{
    state->cb_data = data;
}

void mptcp_state_set_event_cb(struct mptcp_state *state, mptcp_control_cb_t cb, void *data)
{
    state->event_cb = cb;
    state->cb_data = data;
}

mptcp_control_cb_t mptcp_state_get_event_cb(struct mptcp_state *state)
{
    return state->event_cb;
}

int conn_equals(struct connection *new_conn, struct connection *old_conn)
{
    if(!new_conn || !old_conn){
        return -1;
    } else if(new_conn->token != old_conn->token){
        return -1;
    } else if(new_conn->transport != old_conn->transport){
        return -1;
    } else if(new_conn->daddr != old_conn->daddr){
        return -1;
    } else if(new_conn->dport != old_conn->dport){
        return -1;
    }
    return 0;
}


int mptcp_check_local_capability(void)
{
    int fd;
    char line[512];
    ssize_t n;
    char * mptcp_enabled_path = "/proc/sys/net/mptcp/mptcp_enabled";

    memset(line, 0, 512);
    fd = open(mptcp_enabled_path, O_RDONLY);
    if (fd == -1){
         print_error("Failed to open MPTCP file\n");
         return 0;
    }

    n = read(fd, line, 512);
    if (n == -1){
        print_error("Failed to read from MPTCP file\n");
        return 0;
    }

    print_verb("MPTCP Capability: %s\n", line);

    return atoi(line);
}

void mptcp_for_each_connection(struct mptcp_state *mp_state, connection_mod_cb_t cb, void *data)
{
    Litem *item;
    list_for_each(item, mp_state->connections){
        struct connection *c = item->data;
        cb(mp_state, c, data);
    }
}

struct connection *mptcp_find_connection(List *connections, uint32_t saddr, uint32_t daddr, uint32_t dport)
{
    struct connection *conn = (struct connection *)0;
    Litem *item;
    list_for_each(item, connections){
        struct connection *c = item->data;
        print_debug("%lu:%d\n", (unsigned long)c->daddr, c->dport);
        print_debug("%lu:%d\n", (unsigned long)daddr, dport);
        if(c->daddr == daddr
          && c->dport == dport) {
            conn = c;
            break;
        }
    }

    return conn;
}

/*Mutex*/
int mptcp_state_lock(struct mptcp_state *state)
{
    return pthread_mutex_lock(&state->lock);
}

int mptcp_state_unlock(struct mptcp_state *state)
{
    return pthread_mutex_unlock(&state->lock);
}

/*Connections*/

struct connection * mptcp_state_pop_connection_by_token(struct mptcp_state *mp_state, uint32_t token)
{
    struct connection *conn;
    Litem *item = (Litem*)0;
    int i = 0;

    list_for_each(item, mptcp_state_get_connections(mp_state)){
        struct connection *c = item->data;
        if(c && c->token == token){
            List *conns = mptcp_state_get_connections(mp_state);
            if(conns){
                Litem *item = list_remove(conns, i);
                conn = item->data;
            } else {
                print_error("List of connections is null, failed\n");
            }
            break;
        }
        i++;
    }

    return conn;
}

void mptcp_state_put_connection(struct mptcp_state *state, struct connection *c)
{
    Litem *item = (Litem*)0;
    item = malloc(sizeof(Litem));
    item->data = c;
    list_put(state->connections, item);
}

/*Setters*/

void mptcp_state_set_running(struct mptcp_state *state, int running)
{
    state->running = running;
}

void mptcp_state_set_comms(struct mptcp_state *state, void *comm)
{
    state->kernel_comms = comm;
}

/*Getters*/

int mptcp_state_get_running(struct mptcp_state *state)
{
    return state->running;
}

List * mptcp_state_get_connections(struct mptcp_state *state)
{
    return state->connections;
}

void * mptcp_state_get_comms(struct mptcp_state *state)
{
    return state->kernel_comms;
}


/*Memory*/

struct mptcp_state * mptcp_state_alloc(void)
{
    struct mptcp_state *state;
    state = malloc(sizeof(struct mptcp_state));
    state->connections = malloc(sizeof(List));
    list_init(state->connections);
    state->kernel_comms = (void*)0;
    state->running = 0;
    state->event_cb = (mptcp_control_cb_t)0;
    pthread_mutex_init (&(state->lock), NULL);
    return state;
}

void mptcp_state_free(struct mptcp_state *state)
{
    list_destroy(state->connections);
    free(state);
}
