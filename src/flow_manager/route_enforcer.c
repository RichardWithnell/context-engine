#include "route_enforcer.h"
#include "../application_rules.h"
#include "../resource_manager.h"
#include "../iptables/iptables.h"
#include "../path_metrics/path_metric_interface.h"

enum {
    ADD_RULE,
    DELETE_RULE,
    REPLACE_RULE
};

enum {
    NO_MATCH,
    PARTIAL_MATCH,
    FULL_MATCH,
}

struct match_quality {
    int match_type;
    int match_failure;
}

int init_iptables(void)
{
    int ret_val = 0;

    iptables_create_chain("CONTEXT", "mangle");
    iptables_run("/usr/local/sbin/iptables -A OUTPUT -t mangle -j CONTEXT");

    iptables_run("/usr/local/sbin/iptables -A CONTEXT -t mangle -j CONNMARK --restore-mark");
    iptables_run("/usr/local/sbin/iptables -A CONTEXT -t mangle -m mark ! --mark 0 -j ACCEPT");
    iptables_run("/usr/local/sbin/iptables -A CONTEXT -t mangle -j CONNMARK --save-mark");

    return ret_val;
}

/*Returns 0 for ok*/
int resource_requirement_cmp(
  struct application_spec *as,
  struct network_resource *candidate)
{
    int match = FULL_MATCH;
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

    if(bw_req > bw) {
        match = PARTIAL_MATCH;
        fail_count++;
    }

    if(latency_req < latency) {
        match = PARTIAL_MATCH;
        fail_count++;
    }

    if(jitter_req < jitter) {
        match = PARTIAL_MATCH;
        fail_count++;
    }

    if(loss_req < loss) {
        match = PARTIAL_MATCH;
        fail_count++;
    }

    if(fail_count >= 4){
        match = NO_MATCH;
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

int create_iptables_string(struct network_resource *nr, struct application_spec *as, int mode, char *string)
{
    char protocol[4];
    char mod_char = 0;
    unsigned short dport = 0;
    unsigned int mark = 0;
    unsigned int line = 0;
    unsigned int old_line = 0;

    switch(mode){
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
    switch(application_spec_get_protocol(rule)){
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

    sprintf(string,
        "iptables -%c CONTEXT -m mark --mark 0 -p %s --dport %d --ctstate NEW -t mangle -j MARK --set-mark %d",
            mod_char,
            protocol,
            application_spec_get_dport(dport),
            network_resource_get_table(nr));

    return 0;
}

int install_iptables_rule(struct network_resource *nr, struct application_spec *as, List * iptables_rules)
{
    Litem *rule_item;
    struct iptables_mark_rule *rule = (struct iptables_mark_rule*)0;

    rule = iptables_mark_rule_alloc();

    iptables_mark_rule_set_line(rule, line);
    iptables_mark_rule_set_mark(rule, mark);
    iptables_mark_rule_set_daddr(rule, daddr);
    iptables_mark_rule_set_dport(rule, dport);
    iptables_mark_rule_set_table(rule, table);
    iptables_mark_rule_set_chain(rule, chain);

    list_for_each(rule_item, iptables_rules){
        struct iptables_mark_rule *r;

        r = (struct iptables_mark_rule*)rule_item->data;
        if(!iptables_mark_rule_cmp(rule, r)){
            /*Rules Match*/

        }
    }

    return 0;
}

int route_selector(List * resource_list, List * app_specs, List * iptables_rules)
{
    Litem resource_item;
    Litem app_spec_item;
    Litem iptables_rule_item;

    struct network_resource *candidate = (struct network_resource *)0;

    list_for_each(app_spec_item, app_specs){
        struct application_spec *spec = (struct application_spec*)0;
        spec = (struct application_spec*)app_spec_item->data;

        list_for_each(resource_item, resource_list){
            struct network_resource *nr = (struct network_resource*)0;
            nr = (struct network_resource*)resource_item->data;
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
                    if(choose <= 50) {
                        candidate = nr;
                    } else {
                        candidate = candidate;
                    }
                }
            }
        }

        install_iptables_rule(candidate, app_spec, iptables_rules);
    }
}
