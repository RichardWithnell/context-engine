#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "iptables.h"
#include "../util.h"
#include "../debug.h"
#include "../list.h"

enum {
    TCP = 6,
    UDP = 17,
};

enum {
    ADD_RULE,
    DELETE_RULE,
    REPLACE_RULE
};

enum {
    NEW_CHAIN,
    FLUSH_CHAIN,
    DELETE_CHAIN
};

struct iptables_chain_state {
    char name[CHAIN_NAME_LEN];
    char table[TABLE_NAME_LEN];
    int lines;
    int entry;
    int last;
    List *rules;
};


int iptables_run(char * command)
{
    FILE* output = (FILE*)0;
    int status = 0;
    int pid = 0;
    char line[256];

    output = execv_and_pipe("/usr/local/sbin/iptables", command, &pid);

    while(fgets(line, sizeof(line), output)) {
        print_debug("iptables: %s\n", line);
    }
    waitpid(pid, &status, 0);

    return 0;
}

int iptables_run_many(char **run)
{
    int ret = 0;
    while(*run != 0){
        if((ret = iptables_run(*run++))){
            return ret;
        }
    }

    return ret;
}

int iptables_mod_chain(char *chain_name, char *table, int mod)
{
    char rule[512];
    char mod_char = '0';

    switch(mod){
        case NEW_CHAIN:
            mod_char = 'N';
            break;
        case FLUSH_CHAIN:
            mod_char = 'F';
            break;
        case DELETE_CHAIN:
            mod_char = 'X';
            break;
        default:
            break;
    }

    if(mod_char == '0') {
        return -1;
    }

    memset(rule, 0, 512);
    if(table){
        sprintf(rule, "/usr/local/sbin/iptables -t %s -%c %s", table, mod_char, chain_name);
    } else {
        sprintf(rule, "/usr/local/sbin/iptables -%c %s", mod_char, chain_name);
    }

    return iptables_run(rule);
}

int iptables_mod_rule(struct iptables_mark_rule *rule, struct iptables_mark_rule *old_rule, int mode)
{
    char protocol[4];
    char rule_str[512];
    char mod_char = ' ';
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
        case REPLACE_RULE:
            mod_char = 'R';
            break;
        default:
            break;
    }

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

    if(mod_char == ' '){
        print_error("Unknown iptables mod\n");
        return -1;
    }

    mark = iptables_mark_rule_get_mark(rule);
    line = iptables_mark_rule_get_line(rule);
    dport = iptables_mark_rule_get_dport(rule);

    if(old_rule){
        old_line = iptables_mark_rule_get_line(old_rule);
    }

    sprintf(rule_str,
        "iptables -%c %s %d -m mark --mark 0 -p %s --dport %d --ctstate NEW -t %s -j MARK --set-mark %d",
            mod_char,
            iptables_mark_rule_get_chain(rule),
            line,
            protocol,
            dport,
            iptables_mark_rule_get_table(rule),
            mark);

    return iptables_run(rule_str);
}

int iptables_replace_rule(struct iptables_mark_rule *new_rule, struct iptables_mark_rule *old_rule)
{
    if(strcmp(iptables_mark_rule_get_table(new_rule), iptables_mark_rule_get_table(old_rule))){
        print_error("Can't replace rule in different tables\n");
        return -1;
    }

    if(strcmp(iptables_mark_rule_get_chain(new_rule), iptables_mark_rule_get_chain(old_rule))){
        print_error("Can't replace rule in different chain\n");
        return -1;
    }

    return iptables_mod_rule(new_rule, old_rule, REPLACE_RULE);
}

int iptables_delete_rule(struct iptables_mark_rule *rule)
{
    return iptables_mod_rule(rule, (struct iptables_mark_rule*)0, DELETE_RULE);
}

int iptables_add_rule(struct iptables_mark_rule *rule)
{
    return iptables_mod_rule(rule, (struct iptables_mark_rule*)0, ADD_RULE);
}

int iptables_flush_chain(char *chain_name, char *table)
{
    return iptables_mod_chain(chain_name, table, FLUSH_CHAIN);
}

int iptables_delete_chain(char *chain_name, char *table)
{
    return iptables_mod_chain(chain_name, table, DELETE_CHAIN);
}

int iptables_create_chain(char *chain_name, char *table)
{
    return iptables_mod_chain(chain_name, table, NEW_CHAIN);
}
