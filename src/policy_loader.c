#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../cjson/cJSON.h"
#include "policy.h"
#include "debug.h"
#include "util.h"
#include "context_library.h"
#include "action.h"

//List * json_to_policy(jsmntok_t *tokens, char *js);

#define CONDKEY_TO_STR(x) condition_keystr[x]




struct action *parse_json_action(cJSON *json)
{
    struct action *act = (struct action*)0;

    uint32_t action_id = 0;

    cJSON *link_id = (cJSON*)0;
    cJSON *do_act = (cJSON*)0;
    cJSON *mode = (cJSON*)0;

    if(!json){
        return act;
    }

    act = action_alloc();
    if(act) {
        print_verb("Action alloced\n");
    }

    link_id = cJSON_GetObjectItem(json, "link_id");
    if(!link_id) {
        print_error("Could not parse link_id\n");
        return act;
    }

    do_act = cJSON_GetObjectItem(json, "do");
    if(!do_act) {
        print_error("Could not parse do\n");
        return act;
    }

    mode = cJSON_GetObjectItem(json, "mode");
    if(mode) {
        print_verb("Found Mode: %s\n", mode->valuestring);
        return act;
    } else {
        print_error("No Mode Set\n");
    }

    if(!strcmp(mode->valuestring, "hard")){
        action_set_mode(act, ACTION_MODE_HARD);
    } else {
        action_set_mode(act, ACTION_MODE_SOFT);
    }

    action_id = find_action_id(do_act->valuestring);

    action_set_action(act, action_id);
    action_set_link_name(act, link_id->valuestring);



    return act;
}

struct condition *parse_json_condition(cJSON *json, List *context_libs)
{
    struct condition * cond = (struct condition *)0;
    cJSON *jkey = (cJSON*)0;
    cJSON *value = (cJSON*)0;
    cJSON *comparator = (cJSON*)0;
    cJSON *link = (cJSON*)0;

    if(!json){
        print_error("Json is null\n");
        return cond;
    }

    jkey = cJSON_GetObjectItem(json, "key_id");
    if(!jkey){
        print_error("Could not parse key_id\n");
        return cond;
    }

    value = cJSON_GetObjectItem(json, "value");
    if(!value){
        print_error("Could not parse value\n");
        return cond;
    }

    comparator = cJSON_GetObjectItem(json, "comparator");
    if(!comparator){
        print_error("Could not parse comparator\n");
        return cond;
    }

    parse_condition_t pct = context_library_get_parser(context_libs, jkey->valuestring);

    if(!pct){
        print_error("Didn't find parser for condition\n");
        return cond;
    }

    cond = pct(jkey->valuestring, value->valuestring, comparator->valuestring);

    link = cJSON_GetObjectItem(json, "link_id");
    if(link){
        print_verb("Found Link: %s\n", link->valuestring);
    } else {
        print_error("No Link\n");
    }

    if(cond){
        print_debug("Parse condition called successfully\n");
    }

    return cond;
}

List * load_policy_file(char *config_file, List *context_libs)
{
    List *policy_list = (List*)0;
    char *data = (char*)0;
    cJSON *json = (cJSON*)0;;
    int i = 0;
    int j = 0;

    print_verb("\n");

    if(!config_file){
        print_error("Config File is null\n");
        return policy_list;
    }

    data = load_file(config_file);
    if(!data) {
        print_error("Load File Failed\n");
        return policy_list;
    }

    print_verb("Loaded file\n");

	json = cJSON_Parse(data);

	if (!json) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return policy_list;
    }

    policy_list = malloc(sizeof(List));
    list_init(policy_list);

    print_verb("Tokenised\n");


    for (i = 0 ; i < cJSON_GetArraySize(json) ; i++){
        cJSON * subitem = cJSON_GetArrayItem(json, i);
        if(subitem){
            cJSON * p = cJSON_GetObjectItem(subitem, "policy");
            if(p){
                Litem *item;
                struct policy_definition *pd;
                cJSON *condition;
                cJSON *action;

                pd = policy_definition_alloc();

                condition = cJSON_GetObjectItem(p, "condition");
                if(condition){
                    print_verb("Found Condition\n");
                    for (j = 0; j < cJSON_GetArraySize(condition); j++){
                        cJSON * cond = cJSON_GetArrayItem(condition, j);
                        if(cond){
                            struct condition *c = (struct condition *)0;
                            c = parse_json_condition(cond, context_libs);
                            if(c){
                                condition_set_parent(c, pd);
                                policy_definition_put_condition(pd, c);
                                context_library_add_condition(context_libs, c);
                            } else {
                                return (List*)0;
                            }
                        }
                    }
                } else {
                    print_error("No Condition\n");
                }

                action = cJSON_GetObjectItem(p, "action");
                if(action){
                    print_verb("Found Action\n");
                    for (j = 0; j < cJSON_GetArraySize(action); j++){
                        cJSON *act = cJSON_GetArrayItem(action, j);
                        if(act){
                            struct action *a = parse_json_action(act);
                            if(a){
                                policy_definition_put_action(pd, a);
                            } else {
                                return (List*)0;
                            }
                        } else {
                            return (List*)0;
                        }
                    }
                } else {
                    print_error("No Action\n");
                }
                item = malloc(sizeof(Litem));
                item->data = pd;
                list_put(policy_list, item);
            }
        }
    }

    print_verb("Converted to policies\n");

    return policy_list;
}
