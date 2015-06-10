#include "resource_manager.h"
#include "debug.h"
#include "util.h"
#include "list.h"
#include "../cjson/cJSON.h"

/*
[
    {
        "link": {
            "link_id": "eth0",
            "type": "ethernet",
            "mp": "enabled"
        }
    },
    {
        "link": {
            "link_id": "eth2",
            "type": "ethernet",
            "mp": "enabled"
        }
    },
    {
        "link": {
            "link_id": "ppp0",
            "type": "cellular",
            "mp": "backup"
        }
    }
]
*/

enum {
    RULE_MULTIPATH_UNSPEC,
    RULE_MULTIPATH_ENABLED,
    RULE_MULTIPATH_DISABLED,
    RULE_MULTIPATH_HANDOVER,
};

struct physical_link {
    char ifname[IFNAMSIZ];
    uint8_t type;
    uint8_t multipath;
};

#define DEFAULT_MULTIPATH_BEHAVIOUR RULE_MULTIPATH_ENABLED


int parse_physical_link(cJSON *json, struct physical_link *pl)
{
    cJSON *link_id = (cJSON*)0;
    cJSON *type = (cJSON*)0;
    cJSON *multipath = (cJSON*)0;

    link_id = cJSON_GetObjectItem(json, "link_id");
    type = cJSON_GetObjectItem(json, "type");
    multipath = cJSON_GetObjectItem(json, "multipath");

    if(link_id){
        strcpy(pl->ifname, link_id->valuestring);
    } else {
        print_error("Link ID is null\n");
        return -1;
    }

    if(type){
        if(strcmp(type->valuestring, "cellular")){
            pl->type = LINK_TYPE_CELLULAR;
        } else if (strcmp(type->valuestring, "wifi")) {
            pl->type = LINK_TYPE_WIFI;
        } else if (strcmp(type->valuestring, "ethernet")) {
            pl->type = LINK_TYPE_ETHERNET;
        } else if (strcmp(type->valuestring, "wimax")) {
            pl->type = LINK_TYPE_WIMAX;
        } else if (strcmp(type->valuestring, "satellite")) {
            pl->type = LINK_TYPE_SATELLITE;
        } else {
            pl->type = LINK_TYPE_UNSPEC;
        }
    } else {
        pl->type = LINK_TYPE_UNSPEC;
    }

    if(multipath){
        if(strcmp(type->valuestring, "enabled")){
            pl->multipath = RULE_MULTIPATH_ENABLED;
        } else if (strcmp(type->valuestring, "disabled")) {
            pl->multipath = RULE_MULTIPATH_DISABLED;
        } else if (strcmp(type->valuestring, "backup")) {
            pl->multipath = RULE_MULTIPATH_HANDOVER;
        } else {
            pl->multipath = DEFAULT_MULTIPATH_BEHAVIOUR;
        }
    } else {
        pl->multipath = DEFAULT_MULTIPATH_BEHAVIOUR;
    }

    return 0;
}

List * load_link_profiles(char *config_file)
{
    List *phy_list = (List*)0;
    char *data = (char*)0;
    cJSON *json = (cJSON*)0;
    int i = 0;

    print_verb("\n");
    if(!config_file){
        print_error("Config File is null\n");
        return phy_list;
    }

    data = load_file(config_file);
    if(!data) {
        print_error("Load File Failed\n");
        return phy_list;
    }

    print_verb("Loaded file\n");
    json = cJSON_Parse(data);
    if (!json) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return phy_list;
    }

    phy_list = malloc(sizeof(List));
    list_init(phy_list);
    print_verb("Tokenised\n");

    i = 0;
    for (i = 0 ; i < cJSON_GetArraySize(json) ; i++){
        cJSON * subitem = cJSON_GetArrayItem(json, i);
        if(subitem){
            cJSON * link = cJSON_GetObjectItem(subitem, "link");
            if(link){
                Litem *item;
                struct physical_link *pl;

                pl = malloc(sizeof(struct physical_link));
                if(link){
                    parse_physical_link(link, pl);
                } else {
                    print_error("No Link Defined\n");
                    return (List*)0;
                }

                item = malloc(sizeof(Litem));
                item->data = pl;
                list_put(phy_list, item);
            }
        }
    }

    print_verb("Converted to link profiles\n");
    return phy_list;
}

struct physical_link * physical_link_alloc(void)
{
    struct physical_link *pl;
    pl = malloc(sizeof(struct physical_link));
    pl->type = 0;
    pl->multipath = 0;
    memset(pl->ifname, 0, IFNAMSIZ);
    return pl;
}

void physical_link_free(struct physical_link *pl)
{
    free(pl);
}

uint8_t physical_link_get_type(struct physical_link *pl)
{
    return pl->type;
}

char * physical_link_get_ifname(struct physical_link *pl)
{
    return pl->ifname;
}

uint8_t physical_link_get_multipath(struct physical_link *pl)
{
    return pl->multipath;
}

void physical_link_set_type(struct physical_link *pl, uint8_t type)
{
    pl->type = type;
}

void physical_link_set_ifname(struct physical_link *pl, char *ifname)
{
    strcpy(pl->ifname, ifname);
}

void physical_link_set_multipath(struct physical_link *pl, uint8_t multipath)
{
    pl->multipath = multipath;
}
