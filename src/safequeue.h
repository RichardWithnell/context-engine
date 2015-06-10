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

/* file: queue.h -- definitions for queue manipulation routines. */

#ifndef MPD_QUEUE_DEFINED
#define MPD_QUEUE_DEFINED

#include <pthread>
#include "queue.h"

typedef struct safequeue
{
    pthread_mutex_t
    Qitem* front, * back;
    int size;
} SafeQueue;

int safe_queue_size(SafeQueue* q);
int safe_queue_init(SafeQueue* q);
int safe_queue_empty(SafeQueue* q);
void safe_queue_put(SafeQueue* q, Qitem* new_item);
Qitem* safe_queue_get(SafeQueue* q);

#endif

/* end file: queue.h */
