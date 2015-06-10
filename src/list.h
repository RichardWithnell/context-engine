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

#ifndef MPD_LIST
#define MPD_LIST

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct litem;
struct list;


typedef void (*list_cb_t)(struct list *l, struct litem *item, void *data);

typedef struct litem
{
    struct litem* next;
    struct litem* prev;
    void* data;
} Litem;

typedef struct list
{
    Litem* front, * back;
    uint32_t size;
    list_cb_t list_put_cb;
    list_cb_t list_rem_cb;
    void *put_cb_data;
    void *rem_cb_data;
} List;

uint32_t list_size(List* l);
uint32_t list_init(List* l);
uint32_t list_empty(List* l);
void list_put(List* l, Litem* new_item);
void list_destroy(List* l);
Litem* list_remove(List* l, uint32_t index);
Litem* list_get(List* l, uint32_t index);

void list_rem_register_cb(List* l, list_cb_t func, void *data);
void list_put_register_cb(List* l, list_cb_t func, void *data);


#define list_for_each(item, list) \
    for(item = list->front; item != 0; item = item->next) \

#define list_for_each_reverse(item, list) \
    for(item = list->back; item != 0; item = item->prev) \

#endif
