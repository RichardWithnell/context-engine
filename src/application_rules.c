#include <regex.h>
#include <inttypes.h>

#include "application_rules.h"
#include "debug.h"
#include "util.h"
#include "bandwidth_parser.h"
#include "../cjson/cJSON.h"

struct application_spec {
    uint32_t daddr;
    uint32_t saddr;
    uint16_t dport;
    uint16_t sport;
    uint8_t proto;

    uint32_t required_bw;
    double required_latency;
    double required_loss;
    double required_jitter;
    uint8_t multipath;
    uint8_t migrate;
    uint8_t allocate;
};


void print_application_spec(struct application_spec *as)
{
    print_debug("Application Spec\n");
    print_debug("\tdaddr: %"PRIu32"\n", as->daddr);
    print_debug("\tdport: %u\n", as->dport);
    print_debug("\tproto: %u\n", as->proto);
    print_debug("Rule Spec\n");
    print_debug("\tBW: %"PRIu32"\n", as->required_bw);
    print_debug("\tLoss: %f\n", as->required_loss);
    print_debug("\tLatency: %f\n", as->required_latency);
    print_debug("\tJitter: %f\n", as->required_jitter);
    print_debug("\tMP: %u\n", as->multipath);
    print_debug("\tAllocate: %u\n\n", as->allocate);
}

struct application_spec *application_spec_alloc(void)
{
    struct application_spec *as;
    as = malloc(sizeof(struct application_spec));
    as->daddr = 0;
    as->saddr = 0;
    as->dport = 0;
    as->sport = 0;
    as->proto = 0;
    return as;
}

/*
"application":
    {
        "daddr":"google.com"
        "saddr":""
        "dport":"80"
        "sport":""
        "proto":"tcp"
    }
"requirements":
    {
        "bandwidth":"-1"
        "loss":"-1"
        "jitter":"-1"
        "latency":"-1"
        "multipath":"enabled"
    }
*/

int parse_application_spec(cJSON *json, struct application_spec *as)
{
    cJSON *daddr = (cJSON*)0;
    cJSON *saddr = (cJSON*)0;
    cJSON *dport = (cJSON*)0;
    cJSON *sport = (cJSON*)0;
    cJSON *proto = (cJSON*)0;

    daddr = cJSON_GetObjectItem(json, "daddr");
    saddr = cJSON_GetObjectItem(json, "saddr");
    dport = cJSON_GetObjectItem(json, "dport");
    sport = cJSON_GetObjectItem(json, "sport");
    proto = cJSON_GetObjectItem(json, "proto");

    if(daddr){
        uint32_t d = 0;
        if(inet_pton(AF_INET, daddr->valuestring, &d) == 1){
            as->daddr = d;
        }
    }

    if(saddr) {
        uint32_t s = 0;
        if(inet_pton(AF_INET, saddr->valuestring, &s) == 1){
            as->saddr = s;
        }
    }

    if(dport){
        char *ptr;
        uint16_t dprt;
        dprt = strtol(dport->valuestring, &ptr, 10);
        as->dport = htons(dprt);
    }

    if(sport) {
        char *ptr;
        uint16_t sprt;
        sprt = strtol(sport->valuestring, &ptr, 10);
        as->sport = htons(sprt);
    }

    if(proto){
        if(!strcasecmp(proto->valuestring, "TCP")){
            as->proto = TCP;
        } else if (!strcasecmp(proto->valuestring, "UDP")) {
            as->proto = UDP;
        } else {
            as->proto = 0;
        }
    }

    return 0;
}

int parse_requirement_spec(cJSON *json, struct application_spec *as)
{
    char *ptr = (char*)0;
    double l = 0.00;

    cJSON *bandwidth = (cJSON*)0;
    cJSON *loss = (cJSON*)0;
    cJSON *jitter = (cJSON*)0;
    cJSON *latency = (cJSON*)0;
    cJSON *multipath = (cJSON*)0;
    cJSON *allocate = (cJSON*)0;

    bandwidth = cJSON_GetObjectItem(json, "bandwidth");
    loss = cJSON_GetObjectItem(json, "loss");
    jitter = cJSON_GetObjectItem(json, "jitter");
    latency = cJSON_GetObjectItem(json, "latency");
    multipath = cJSON_GetObjectItem(json, "multipath");
    allocate = cJSON_GetObjectItem(json, "allocate");

    if(bandwidth){
        if(!validate_bandwidth_value(bandwidth->valuestring)){
            char *unit_string = (char*)0;
            uint32_t bw = get_bandwidth(bandwidth->valuestring);
            unit_string = get_bandwidth_unit(bandwidth->valuestring);

            if(strcasecmp(unit_string, "b")){
                as->required_bw = (uint32_t)bw;
            } else if(strcasecmp(unit_string, "kb")){
                as->required_bw = KB_TO_B(bw);
            } else if(strcasecmp(unit_string, "mb")){
                as->required_bw = MB_TO_B(bw);
            } else if(strcasecmp(unit_string, "gb")){
                as->required_bw = GB_TO_B(bw);
            }
        } else {
            as->required_bw = 0;
        }
    }

    if(loss){
        l = strtof(loss->valuestring, &ptr);
        as->required_loss = l;
    } else {
        printf("No Loss Parameter Set\n");
    }

    if(jitter){
        l = strtof(loss->valuestring, &ptr);
        as->required_jitter = l;
    } else {
        printf("No Jitter Parameter Set\n");
    }

    if(latency){
        l = strtof(latency->valuestring, &ptr);
        as->required_latency = l;
    } else {
        printf("No Latency Parameter Set\n");
    }

    if(multipath){
        if(!strcasecmp(multipath->valuestring, "enabled")){
            as->multipath = RULE_MULTIPATH_ENABLED;
        } else if(!strcasecmp(multipath->valuestring, "disabled")){
            as->multipath = RULE_MULTIPATH_DISABLED;
        }
    } else {
        as->multipath = DEFAULT_MULTIPATH_BEHAVIOUR;
        print_error("No Multipath Parameter Set\n");
    }

    if(allocate){
        if(!strcasecmp(allocate->valuestring, "enabled")){
            as->allocate = RULE_ALLOCATE_RESOURCE_YES;
        } else if(!strcasecmp(multipath->valuestring, "disabled")) {
            as->allocate = RULE_ALLOCATE_RESOURCE_NO;
        }
    } else {
        as->allocate = RULE_ALLOCATE_RESOURCE_NO;
        print_error("No Allocation Parameter Set\n");
    }

    return 0;
}

List * load_application_specs(char *config_file)
{
    List *spec_list = (List*)0;
    char *data = (char*)0;
    cJSON *json = (cJSON*)0;;
    int i = 0;

    print_verb("\n");

    if(!config_file){
        print_error("Config File is null\n");
        return spec_list;
    }

    data = load_file(config_file);
    if(!data) {
        print_error("Load File Failed\n");
        return spec_list;
    }

    print_verb("Loaded file\n");

    json = cJSON_Parse(data);

    if (!json) {
        printf("Error before: [%s]\n", cJSON_GetErrorPtr());
        return spec_list;
    }

    spec_list = malloc(sizeof(List));
    list_init(spec_list);

    print_verb("Tokenised\n");

    i = 0;
    for (i = 0 ; i < cJSON_GetArraySize(json) ; i++){
        cJSON * subitem = cJSON_GetArrayItem(json, i);
        if(subitem){
            cJSON * spec = cJSON_GetObjectItem(subitem, "spec");
            if(spec){
                Litem *item;
                struct application_spec *as;
                cJSON *app_spec;
                cJSON *requirement;

                as = malloc(sizeof(struct application_spec));

                app_spec = cJSON_GetObjectItem(spec, "application");
                if(app_spec){
                    parse_application_spec(app_spec, as);
                } else {
                    print_error("No Application Spec Defined\n");
                    return (List*)0;
                }

                requirement = cJSON_GetObjectItem(spec, "requirements");
                if(requirement){
                    parse_requirement_spec(requirement, as);
                } else {
                    print_error("No Application Spec Defined\n");
                    return (List*)0;
                }
                print_application_spec(as);
                item = malloc(sizeof(Litem));
                item->data = as;
                list_put(spec_list, item);
            }
        }
    }

    print_verb("Converted to application specs\n");
    return spec_list;
}

void application_spec_free(struct application_spec *as)
{
    free(as);
}

uint8_t application_spec_get_allocate(struct application_spec *as)
{
    return as->allocate;
}

void application_spec_set_required_bw(struct application_spec *as, uint32_t bw)
{
    as->required_bw = bw;
}

void application_spec_set_required_latency(struct application_spec *as, double latency)
{
    as->required_latency = latency;
}

void application_spec_set_required_loss(struct application_spec *as, double loss)
{
    as->required_loss = loss;
}

void application_spec_set_required_jitter(struct application_spec *as, double jitter)
{
    as->required_jitter = jitter;
}

void application_spec_set_multipath(struct application_spec *as, uint8_t mp)
{
    as->multipath = mp;
}

void application_spec_set_daddr(struct application_spec *as, uint32_t daddr)
{
    as->daddr = daddr;
}

void application_spec_set_saddr(struct application_spec *as, uint32_t saddr)
{
    as->saddr = saddr;
}

void application_spec_set_dport(struct application_spec *as, uint16_t dport)
{
    as->dport = dport;
}

void application_spec_set_sport(struct application_spec *as, uint16_t sport)
{
    as->sport = sport;
}

void application_spec_set_proto(struct application_spec *as, uint8_t proto)
{
    as->proto = proto;
}


uint32_t application_spec_get_daddr(struct application_spec *as)
{
    return as->daddr;
}

uint32_t application_spec_get_saddr(struct application_spec *as)
{
    return as->saddr;
}

uint16_t application_spec_get_dport(struct application_spec *as)
{
    return as->dport;
}

uint16_t application_spec_get_sport(struct application_spec *as)
{
    return as->sport;
}

uint8_t application_spec_get_proto(struct application_spec *as)
{
    return as->proto;
}

uint32_t application_spec_get_required_bw(struct application_spec *as)
{
    return as->required_bw;
}

double application_spec_get_required_latency(struct application_spec *as)
{
    return as->required_latency;
}


double application_spec_get_required_loss(struct application_spec *as)
{
    return as->required_loss;
}

double application_spec_get_required_jitter(struct application_spec *as)
{
    return as->required_jitter;
}

uint8_t application_spec_get_multipath(struct application_spec *as)
{
    return as->multipath;
}
