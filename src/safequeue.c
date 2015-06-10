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

#include "queue.h"

/**
 *
 */
int
safe_queue_init(SafeQueue* q)
{
    q->front = q->back = (Qitem*)0;
    q->size = 0;
    return 0;
}

/**
 *
 */
int
safe_queue_empty(SafeQueue* q)
{
    return q->front == (Qitem*)0;
}

/**
 *
 */
int
safe_queue_size(Queue* q)
{
    return q->size;
}

/**
 *
 */
void
safe_queue_put(SafeQueue* q, Qitem* new_item)
{
    new_item->next = (Qitem*)0;
    if (queue_empty(q)) {
        q->front = new_item;
    } else {
        q->back->next = new_item;
    }
    q->back = new_item;
    q->size++;
}

/**
 *
 */
Qitem*
safe_queue_get(SafeQueue* q)
{
    queue_get(&q->q);
}

/* end file: queue.c */
