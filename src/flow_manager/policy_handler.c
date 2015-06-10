#include <pthread.h>

#include "policy_handler.h"
#include "../debug.h"

struct policy_handler_state {
    struct mptcp_state *mp_state;

    pthread_t mptcp_thread;
};

struct policy_handler_state * policy_handler_init(void)
{
    struct mptcp_state *mp_state = (struct mptcp_state*)0;
    pthread_t mptcp_thread;

    if(mptcp_check_local_capability()){
        print_debug("Host is MP-Capable");
        mp_state = mptcp_state_alloc();
        pthread_create(&mptcp_thread, 0,mptcp_control_start, mp_state);

    } else {
        print_error("Host is not MP-Capable\n");
    }



    return (void*)0;
}
