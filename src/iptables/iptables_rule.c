#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "iptables.h"

struct iptables_mark_rule {
    uint8_t line;
    uint8_t mark;
    uint8_t protocol;
    uint32_t daddr;
    uint16_t dport;
    char table[TABLE_NAME_LEN];
    char chain[CHAIN_NAME_LEN];
};

int iptables_mark_rule_cmp(struct iptables_mark_rule *rule_one, struct iptables_mark_rule *rule_two)
{
    int diff = 0;

    if(rule_one->protocol == rule_two->protocol){
        diff++;
    }

    if(rule_one->daddr == rule_two->daddr){
        diff++;
    }

    if(rule_one->dport == rule_two->dport){
        diff++;
    }

    if(strcmp(rule_one->table, rule_two->table)){
        diff++;
    }

    if(strcmp(rule_one->chain, rule_two->chain)){
        diff++;
    }

    return diff;
}

/*Setters*/

void iptables_mark_rule_set_protocol(struct iptables_mark_rule *rule, uint16_t protocol)
{
    rule->protocol = protocol;
}

void iptables_mark_rule_set_line(struct iptables_mark_rule *rule, uint8_t line)
{
    rule->line = line;
}

void iptables_mark_rule_set_mark(struct iptables_mark_rule *rule, uint8_t mark)
{
    rule->mark = mark;
}

void iptables_mark_rule_set_daddr(struct iptables_mark_rule *rule, uint32_t daddr)
{
    rule->daddr = daddr;
}

void iptables_mark_rule_set_dport(struct iptables_mark_rule *rule, uint16_t dport)
{
    rule->dport = dport;
}

void iptables_mark_rule_set_table(struct iptables_mark_rule *rule, char *table)
{
    strncpy(rule->table, table, TABLE_NAME_LEN-1);
}

void iptables_mark_rule_set_chain(struct iptables_mark_rule *rule, char *chain)
{
    strncpy(rule->chain, chain, CHAIN_NAME_LEN-1);
}

/*Getters*/

uint16_t iptables_mark_rule_get_protocol(struct iptables_mark_rule *rule)
{
    return rule->protocol;
}

uint8_t iptables_mark_rule_get_line(struct iptables_mark_rule *rule)
{
    return rule->line;
}

uint8_t iptables_mark_rule_get_mark(struct iptables_mark_rule *rule)
{
    return rule->mark;
}

uint32_t iptables_mark_rule_get_daddr(struct iptables_mark_rule *rule)
{
    return rule->daddr;
}

uint16_t iptables_mark_rule_get_dport(struct iptables_mark_rule *rule)
{
    return rule->dport;
}

char * iptables_mark_rule_get_table(struct iptables_mark_rule *rule)
{
    return rule->table;
}

char * iptables_mark_rule_get_chain(struct iptables_mark_rule *rule)
{
    return rule->chain;
}

/*Memory*/

void iptables_mark_rule_free(struct iptables_mark_rule *rule)
{
    free(rule);
}

struct iptables_mark_rule * iptables_mark_rule_alloc(void)
{
    struct iptables_mark_rule *rule;
    rule = malloc(sizeof(struct iptables_mark_rule));
    memset(rule->table, 0, TABLE_NAME_LEN);
    memset(rule->chain, 0, CHAIN_NAME_LEN);

    rule->line = 0;
    rule->mark = 0;
    rule->daddr = 0;
    rule->dport = 0;
    rule->protocol = 0;

    return rule;
}
