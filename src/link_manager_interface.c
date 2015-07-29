#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include "link_manager_interface.h"
#include "mnl_link_manager.h"
#include "exec_link_manager.h"

int link_manager_down(char *ifname)
{
    return exec_link_manager_down(ifname);
}

int link_manager_up(char *ifname)
{
    return exec_link_manager_up(ifname);
}

void * async_link_down(void *data)
{
    char *ifname = (char*)0;
    ifname = (char*)data;
    if(!ifname) return (void*)0;
    link_manager_down(ifname);
    return (void*)0;
}

void * async_link_up(void *data)
{
    char *ifname = (char*)0;
    ifname = (char*)data;
    if(!ifname) return (void*)0;
    link_manager_up(ifname);
    return (void*)0;
}

int async_link_manager_down(char *ifname)
{
    pthread_t thread;
    if(!ifname) return -1;
    pthread_create(&thread, 0, (void*)&async_link_down, (void*)ifname);
    return 0;
}

int async_link_manager_up(char *ifname)
{
    pthread_t thread;
    if(!ifname) return -1;
    pthread_create(&thread, 0, (void*)&async_link_up, (void*)ifname);
    return 0;
}
