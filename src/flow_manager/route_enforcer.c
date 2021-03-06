#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#ifdef USE_PCA
#include "../path_selection/pca_ps.h"
#endif

#include "../list.h"
#include "route_enforcer.h"
#include "../resource_manager.h"
#include "../iptables/iptables.h"
#include "../path_metrics/path_metric_interface.h"
#include "../util.h"

enum {
    ADD_RULE,
    DELETE_RULE,
    INSERT_RULE,
    REPLACE_RULE
};

enum {
    NO_MATCH,
    PARTIAL_MATCH,
    FULL_MATCH,
};

struct match_quality {
    int match_type;
    int match_failure;
};

#define SPECIFIC_RULE_ADD_POSITION 3

int iptables_remove_snat(void)
{
    FILE* output = (FILE*)0;
    int status = 0;
    int pid = 0;
    char line[512];
    char rule[512];
    char address[32];
    char mark[16];
    char *cmd = (char*)0;

    cmd = malloc(strlen("iptables -t nat -L POSTROUTING")+1);

    strcpy(cmd, "iptables -t nat -L POSTROUTING");

    output = execv_and_pipe("iptables", cmd, &pid);

    if(!output){
        print_error("Could not exec iptables\n");
        return -1;
    }

    while(fgets(line, sizeof(line), output)) {
        if(strstr(line, "SNAT")){
            char *match;
            memset(address, 0, 32);
            memset(mark, 0, 16);
            memset(rule, 0, 512);

            match = get_regex_match(line, "0x[[:xdigit:]]+");
            if(!match){
                print_error("Failed to extract mark\n");
                continue;
            }
            strcpy(mark, match);

            match = get_regex_match(line,
                        "([0-9]+)\\.([0-9]+)\\.([0-9]+)\\.([0-9]+)");
            if(!match){
                print_error("Failed to extract IP address\n");
                continue;
            }
            strcpy(address, match);

            sprintf(rule, "iptables -t nat -D POSTROUTING -m mark --mark %s -j SNAT --to-source %s",  mark, address);

            printf("Delete RULE: %s\n", rule);

            iptables_run(rule);
        }
    }
    waitpid(pid, &status, 0);

    free(cmd);

    return 0;
}


int init_iptables_context(void)
{
    int ret_val = 0;
    char *command = malloc(sizeof(char)*512);

    memset(command, 0, 512);

    /*
    /usr/local/sbin/iptables -D OUTPUT -t mangle -j CONTEXT

    /usr/local/sbin/iptables -A OUTPUT -t mangle -j CONTEXT
    /usr/local/sbin/iptables -A CONTEXT -t mangle -j CONNMARK --restore-mark
    /usr/local/sbin/iptables -A CONTEXT -t mangle -m mark ! --mark 0 -j ACCEPT
    iptables -I CONTEXT 3 -m mark --mark 0 -s 0.0.0.0 -p tcp --dport 5001 -m conntrack --ctstate NEW -t mangle -j MARK --set-mark 4
    /usr/local/sbin/iptables -A CONTEXT -t mangle -j CONNMARK --save-mark
    */

    iptables_remove_snat();

    iptables_run("iptables -D OUTPUT -t mangle -j CONTEXT");

    iptables_flush_chain("CONTEXT", "mangle");
    iptables_delete_chain("CONTEXT", "mangle");
    iptables_create_chain("CONTEXT", "mangle");

    iptables_run("iptables -A OUTPUT -t mangle -j CONTEXT");
    iptables_run("iptables -A CONTEXT -t mangle -j CONNMARK --restore-mark");
    iptables_run("iptables -A CONTEXT -t mangle -m mark ! --mark 0 -j ACCEPT");
    iptables_run("iptables -A CONTEXT -t mangle -j CONNMARK --save-mark");

    return ret_val;
}


/*Returns 0 for ok*/
int resource_requirement_cmp(
  struct application_spec *as,
  struct network_resource *candidate)
{
    int fail_count = 0;

    struct path_stats *ps;
    uint32_t bw = 0;
    double jitter = 0.00;
    double loss = 0.00;
    double latency = 0.00;

    uint32_t bw_req = 0;
    double jitter_req = 0.00;
    double loss_req = 0.00;
    double latency_req = 0.00;

    ps = network_resource_get_state(candidate);

    bw = metric_get_bandwidth(ps);
    latency = metric_get_delay(ps);
    jitter = metric_get_jitter(ps);
    loss =  metric_get_loss(ps);

    bw_req = application_spec_get_required_bw(as);
    latency_req = application_spec_get_required_latency(as);
    loss_req = application_spec_get_required_loss(as);
    jitter_req = application_spec_get_required_jitter(as);

    if(bw_req > bw && bw_req > 0) {
        fail_count++;
    }

    if(latency_req < latency && latency_req >= 0) {
        fail_count++;
    }

    if(jitter_req < jitter && jitter_req >= 0) {
        fail_count++;
    }

    if(loss_req < loss && loss_req >= 0) {
        fail_count++;
    }

    return fail_count;
}


/*Return 0 for the same, 1 for candidate_one and 2 for candidate_two*/
double metric_comparison(
  struct application_spec *as,
  struct network_resource *candidate_one,
  struct network_resource *candidate_two)
{
    int res_one = 0;
    int res_two = 0;

    res_one = resource_requirement_cmp(as, candidate_one);
    res_two = resource_requirement_cmp(as, candidate_two);

    if(res_one < res_two){
        return 1;
    } else if(res_two < res_one) {
        return 2;
    }

    return 0;
}

int create_iptables_string(struct iptables_mark_rule *rule, int mode, char *string)
{
    char protocol[4];
    char mod_char = 0;
    char *string_builder = (char*)0;
    char *tmp = (char*)0;

    switch(mode){
        case INSERT_RULE:
            mod_char = 'I';
            break;
        case ADD_RULE:
            mod_char = 'A';
            break;
        case DELETE_RULE:
            mod_char = 'D';
            break;
        default:
            break;
    }

    if(mod_char == 0){
        print_debug("Invalid rule mod mode\n");
        return -1;
    }

    memset(protocol, 0, 4);
    switch(iptables_mark_rule_get_protocol(rule)){
        case TCP:
            strncpy(protocol, "TCP", 3);
            break;
        case UDP:
            strncpy(protocol, "UDP", 3);
            break;
        default:
            break;
    }

    if(strlen(protocol) < 3) {
        print_debug();
        return -1;
    }

    string_builder = malloc(sizeof(char)*512);
    memset(string_builder, 0, 512);
    tmp = malloc(sizeof(char)*512);
    memset(tmp, 0, 512);

    /*
    "iptables -I CONTEXT 3 -m mark --mark 0 -s 0.0.0.0 -p tcp --dport 5001 -m conntrack --ctstate NEW -t mangle -j MARK --set-mark 4"
    */
    if(mod_char == 'I'){
        sprintf(string_builder, "iptables -%c CONTEXT 3 -m mark --mark 0 -s 0.0.0.0 -p %s", mod_char, protocol);
    } else {
        sprintf(string_builder, "iptables -%c CONTEXT -m mark --mark 0 -s 0.0.0.0 -p %s", mod_char, protocol);
    }

    if(iptables_mark_rule_get_dport(rule)){
        sprintf(tmp, " --dport %d", iptables_mark_rule_get_dport(rule));
        strcat(string_builder, tmp);
    }

    if(iptables_mark_rule_get_daddr(rule)){
        sprintf(tmp, " --daddr %s", ip_to_str(htonl(iptables_mark_rule_get_daddr(rule))));
        strcat(string_builder, tmp);
    }

    strcat(string_builder, " -m conntrack --ctstate NEW -t mangle");

    if(mode == ADD_RULE || mode == INSERT_RULE){
        sprintf(tmp, "-j MARK --set-mark %d", iptables_mark_rule_get_mark(rule));
        strcat(string_builder, tmp);
    }

    strcpy(string, string_builder);

    print_verb("Created iptables command: %s\n", string);

    free(tmp);
    free(string_builder);
    return 0;
}

int create_and_run_iptables(struct iptables_mark_rule *rule, int mode)
{
    char *string = (char*)0;
    string = malloc(sizeof(char)*512);
    create_iptables_string(rule, mode, string);
    iptables_run(string);
    free(string);
    return 0;
}

int install_iptables_rule(struct network_resource *nr, struct application_spec *as, List * iptables_rules)
{
    Litem *rule_item;
    struct iptables_mark_rule *rule = (struct iptables_mark_rule*)0;

    rule = iptables_mark_rule_alloc();

    iptables_mark_rule_set_line(rule, SPECIFIC_RULE_ADD_POSITION);
    iptables_mark_rule_set_mark(rule, network_resource_get_table(nr));
    iptables_mark_rule_set_daddr(rule, application_spec_get_daddr(as));
    iptables_mark_rule_set_dport(rule, application_spec_get_dport(as));
    iptables_mark_rule_set_table(rule, "mangle");
    iptables_mark_rule_set_chain(rule, "CONTEXT");

    list_for_each(rule_item, iptables_rules){
        struct iptables_mark_rule *r;

        r = (struct iptables_mark_rule*)rule_item->data;
        if(!iptables_mark_rule_cmp(rule, r)){
            /*Rules Match*/
            create_and_run_iptables(r, DELETE_RULE);
            create_and_run_iptables(rule, INSERT_RULE);
            rule_item->data = rule;
            free(r);
            return 0;
        }
    }

    rule_item = malloc(sizeof(Litem));
    rule_item->data = rule;
    list_put(iptables_rules, rule_item);

    return 0;
}

struct network_resource * route_selector_resource_loop(
  List *resource_list,
  struct application_spec *spec,
  int try_backup)
{

#ifdef USE_PCA
    uint32_t address = 0;
    Litem *resource_item;

    if(list_size(resource_list) <= 0){
        return (struct network_resource *)0;
    }

    if(list_size(resource_list) == 1){
        struct network_resource *nr = (struct network_resource *)(resource_list->front->data);
        return nr;
    }

    address = pca_path_selection(resource_list, spec);

    if(!address){
        return (struct network_resource *)0;
    }

    list_for_each(resource_item, resource_list){
        struct network_resource *nr = (struct network_resource*)0;
        if(address == network_resource_get_address(nr)){
            return (struct network_resource *)nr;
        }
    }

    return (struct network_resource *)0;

#else

    Litem *resource_item;
    struct network_resource *candidate = (struct network_resource *)0;

    list_for_each(resource_item, resource_list){
        struct network_resource *nr = (struct network_resource*)0;
        nr = (struct network_resource*)resource_item->data;
        if(network_resource_get_available(nr) == RESOURCE_UNAVAILABLE){
            continue;
        }
        if(network_resource_get_available(nr) == RESOURCE_BACKUP && try_backup == 0){
            continue;
        }
        if(!candidate){
            candidate = nr;
        } else {
            int res = 0;
            res = metric_comparison(spec, nr, candidate);
            if(res == 1) {
                candidate = nr;
            } else if(res == 2) {
                candidate = candidate;
            } else {
                int choose = rand() % 100;
                print_verb("Links are equal, choose randomly\n");
                if(choose <= 50) {
                    candidate = nr;
                } else {
                    candidate = candidate;
                }
            }
        }
    }

    return candidate;
#endif
}

int route_selector(List * resource_list, List * app_specs, List * iptables_rules)
{
    Litem *app_spec_item;

    struct network_resource *candidate = (struct network_resource *)0;

    list_for_each(app_spec_item, app_specs){
        struct application_spec *spec = (struct application_spec*)0;
        spec = (struct application_spec*)app_spec_item->data;

        if(!spec){
            print_error("Application Spec is null\n");
            continue;
        }

        if(application_spec_get_allocate(spec) != RULE_ALLOCATE_RESOURCE_YES){
            print_verb("Not allocating resource to link, trust the application chose the source\n");
            continue;
        }

        candidate = route_selector_resource_loop(resource_list, spec, 0);
        if(!candidate) {
            route_selector_resource_loop(resource_list, spec, 1);
        }

        if(candidate){
            print_log("Chosen Link %s (%zu)\n",
                network_resource_get_ifname(candidate),
                htonl(network_resource_get_address(candidate)));

            print_log("\tfor App Spec: %zu:%d\n",
                htonl(application_spec_get_daddr(spec)),
                application_spec_get_dport(spec));

            install_iptables_rule(candidate, spec, iptables_rules);
        } else {
            print_error("No links are available for connection\n");
        }
    }
    return 0;
}
