#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#include "mptcp_state.h"
#include "../util.h"
#include "../debug.h"

struct mptcp_state {
    List *connections;
    void *kernel_comms;
    int running;
    mptcp_control_cb_t event_cb;
    void *cb_data;
    pthread_mutex_t lock;
};

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
    int enabled = 0;

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
    return pthread_mutex_lock(&state->lock);
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
            Litem *item = list_remove(mptcp_state_get_connections(mp_state), i);
            conn = item->data;
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
