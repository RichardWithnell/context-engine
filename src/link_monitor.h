/*
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    Author: Richard Withnell
    github.com/richardwithnell
 */

#ifndef MPD_LINK_MONITOR
#define MPD_LINK_MONITOR

#include <netlink/addr.h>
#include <netlink/cache.h>
#include <netlink/data.h>
#include <netlink/netlink.h>
#include <netlink/netlink.h>
#include <netlink/object.h>
#include <netlink/route/addr.h>
#include <netlink/route/link.h>
#include <netlink/route/nexthop.h>
#include <netlink/route/route.h>
#include <netlink/route/rtnl.h>
#include <netlink/route/rule.h>
#include <netlink/utils.h>
#include <pthread.h>
#include <semaphore.h>

#include "queue.h"

struct cache_monitor
{
    Queue* queue;
    pthread_mutex_t* lock;
    sem_t* barrier;
    struct nl_cache* addr_cache;
    struct nl_cache* link_cache;
    struct nl_cache* route_cache;
};

struct update_obj
{
    int action;
    int type;
    void* update;
};

enum {
    UPDATE_LINK = 0x01,
    UPDATE_ADDR = 0x02,
    UPDATE_ROUTE = 0x03,
    UPDATE_GATEWAY = 0x04
};

enum {
    ADD_IFF = 0x01,
    DEL_IFF = 0x02,
    ADD_IP = 0x03,
    DEL_IP = 0x04,
    CHANGE_IP = 0x07,
    CHANGE_RT = 0x08,
    ADD_RT = 0x05,
    DEL_RT = 0x06
};

void init_monitor(void* data);

#endif

/* end file: link_monitor.h */
