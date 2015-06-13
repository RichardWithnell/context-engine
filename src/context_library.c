#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dlfcn.h>
#include <pthread.h>

#include "context_library.h"
#include "list.h"

struct context_library
{
    unsigned int id;
    char path[CONTEXT_LIBRARY_PATH_SIZE];
    List *keys;
    List *conditions;
    init_condition_t init;
    parse_condition_t parser;
    start_context_lib_t start;
    pthread_t thread;
};

int register_key_cb(char *key, void *data)
{
    struct context_library *ch = (struct context_library *)0;
    Litem *item = (Litem*)0;

    if(!data){
        return -1;
    }

    ch = (struct context_library *)data;

    print_verb("Registering Key: %s to %s\n", key, ch->path);

    item = malloc(sizeof(Litem));
    item->data = malloc(strlen(key)+1);
    memset(item->data, 0, strlen(key)+1);
    strncpy(item->data, key, strlen(key));

    list_put(ch->keys, item);

    return 0;
}

int context_library_start(List *context_libs, condition_cb_t cb, void *data)
{
    Litem *item;
    list_for_each(item, context_libs){
        struct context_library *lib = (struct context_library*)item->data;
        struct condition_context *cc = malloc(sizeof(struct condition_context));
        cc->cb = cb;
        cc->data = data;
        cc->conditions = lib->conditions;
        print_debug("%s: %d\n", lib->path, list_size(lib->conditions));
        pthread_create(&(lib->thread), NULL, lib->start, cc);
    }

    return 0;
}

int load_context_libs(List *list, char *libs[])
{
    int i = 0;
    while(libs[i] != 0){
        Litem *item = (Litem*)0;
        struct context_library *ch = (struct context_library*)0;

        if(context_library_check_loaded(list, libs[i])){
            print_debug("Library already loaded: %s\n", libs[i]);
            continue;
        }

        ch = context_library_alloc();

        context_library_set_path(ch, libs[i]);
        context_library_set_id(ch, i);

        if(context_library_load(ch)){
            print_error("Failed to load lib: %s\n", libs[i]);
            context_library_free(ch);
            return -1;
        } else {
            item = malloc(sizeof(Litem));
            item->data = ch;
            list_put(list, item);

            ch->init(register_key_cb, ch);
        }
        i++;
    }

    return 0;
}

int context_library_set_condition_cb(struct context_library *cl, condition_cb_t cb, void *data)
{
    /*
    Litem *item;
    list_for_each(item, cl->conditions){
        struct condition *c = (struct condition*)item->data;
        condition_set_callback(c, cb, data);
    }
    return 0;
    */
    print_error("NOT IMPLEMENTED.");
    return 0;
}


int context_library_add_condition(List *context_libs, struct condition *cond)
{
    struct context_library *cl = (struct context_library*)0;

    cl = context_library_get_for_key(context_libs, condition_get_key(cond));
    print_verb("[%s] context_lib -> %s\n", condition_get_key(cond), cl->path);
    if(cl){
        Litem *item = malloc(sizeof(Litem));
        item->data = cond;
        list_put(cl->conditions, item);
        return 0;
    } else {
        return -1;
    }
}

struct context_library * context_library_get_for_key(List *list, char * key)
{
    Litem *citem;
    list_for_each(citem, list){
        Litem *kitem;
        struct context_library *c = (struct context_library*)citem->data;
        list_for_each(kitem, c->keys){
            char *k = (char*)kitem->data;
            if(!strncmp(k, key, strlen(key))){
                return c;
            }
        }
    }
    return (struct context_library *)0;
}


parse_condition_t context_library_get_parser(List *list, char * key)
{
    Litem *citem;
    list_for_each(citem, list){
        Litem *kitem;
        struct context_library* c;
        c = (struct context_library*)citem->data;
        print_verb("Checking: %s [keys - %d]\n", c->path, list_size(c->keys));

        list_for_each(kitem, c->keys){
            char *k = (char*)kitem->data;
            print_verb("Comparing: %s and %s\n", key, k);
            if(!strncmp(k, key, strlen(key))){
                print_verb("Found Library for parser: %s\n", c->path);
                return c->parser;
            }
        }
    }
    return (parse_condition_t)0;
}

int context_library_check_loaded(List *list, char *path)
{
    Litem *item;
    list_for_each(item, list){
        struct context_library *lib = item->data;
        if(!strcmp(lib->path, path)){
            return -1;
        }
    }

    return 0;
}

int context_library_load(struct context_library *ch)
{
    void *handle;
    char *error;

    handle = dlopen (ch->path, RTLD_LAZY);
    if (!handle) {
        print_error("Failed to open library: %s\n", dlerror());
        return -1;
    }

    ch->init = dlsym(handle, "init_condition");
    if ((error = dlerror()) != NULL)  {
        print_error("Failed to find symbol: %s\n", dlerror());
        return -1;
    }

    ch->parser = dlsym(handle, "parse_condition");
    if ((error = dlerror()) != NULL)  {
        print_error("Failed to find symbol: %s\n", dlerror());
        return -1;
    }

    ch->start = dlsym(handle, "start");
    if ((error = dlerror()) != NULL)  {
        print_error("Failed to find symbol: %s\n", dlerror());
        return -1;
    }


    return 0;
}

struct context_library *context_library_alloc(void)
{
    struct context_library *ch = (struct context_library*)0;
    ch = malloc(sizeof(struct context_library));
    memset(ch->path, 0, CONTEXT_LIBRARY_PATH_SIZE);
    ch->init = (init_condition_t)0;
    ch->id = 0;

    ch->keys = malloc(sizeof(List));
    list_init(ch->keys);

    ch->conditions = malloc(sizeof(List));
    list_init(ch->conditions);

    return ch;
}

void context_library_free(struct context_library *ch)
{
    free(ch);
}

void context_library_set_id(struct context_library *ch, int id)
{
    ch->id = id;
}

void context_library_set_path(struct context_library *ch, char* path)
{
    strncpy(ch->path, path, strlen(path));
}

void context_library_set_init(struct context_library *ch, init_condition_t init)
{
    ch->init = init;
}

int context_library_get_id(struct context_library *ch)
{
    return ch->id;
}

char * context_library_get_path(struct context_library *ch)
{
    return ch->path;
}

init_condition_t context_library_get_init(struct context_library *ch)
{
    return ch->init;
}
