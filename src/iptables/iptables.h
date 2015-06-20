#ifndef IPTABLES_LIB
#define IPTABLES_LIB

#include <stdint.h>

#define TABLE_NAME_LEN 32
#define CHAIN_NAME_LEN 32

struct iptables_mark_rule;
struct iptables_chain_state;

int iptables_mark_rule_cmp(struct iptables_mark_rule *rule_one, struct iptables_mark_rule *rule_two);

/*setters*/
void iptables_mark_rule_set_line(struct iptables_mark_rule *rule, uint8_t line);
void iptables_mark_rule_set_mark(struct iptables_mark_rule *rule, uint8_t mark);
void iptables_mark_rule_set_daddr(struct iptables_mark_rule *rule, uint32_t daddr);
void iptables_mark_rule_set_dport(struct iptables_mark_rule *rule, uint16_t dport);
void iptables_mark_rule_set_protocol(struct iptables_mark_rule *rule, uint16_t protocol);
void iptables_mark_rule_set_table(struct iptables_mark_rule *rule, char *table);
void iptables_mark_rule_set_chain(struct iptables_mark_rule *rule, char *chain);

/*Getters*/
uint8_t iptables_mark_rule_get_line(struct iptables_mark_rule *rule);
uint8_t iptables_mark_rule_get_mark(struct iptables_mark_rule *rule);
uint32_t iptables_mark_rule_get_daddr(struct iptables_mark_rule *rule);
uint16_t iptables_mark_rule_get_dport(struct iptables_mark_rule *rule);
uint16_t iptables_mark_rule_get_protocol(struct iptables_mark_rule *rule);
char * iptables_mark_rule_get_table(struct iptables_mark_rule *rule);
char * iptables_mark_rule_get_chain(struct iptables_mark_rule *rule);

/*Memory*/
void iptables_mark_rule_free(struct iptables_mark_rule *rule);
struct iptables_mark_rule * iptables_mark_rule_alloc(void);

/*Function*/
int iptables_flush_chain(char *chain_name, char *table);
int iptables_delete_chain(char *chain_name, char *table);
int iptables_create_chain(char *chain_name, char *table);
int iptables_run(char * command);

int iptables_add_snat(char *ip_addr, int mark);
int iptables_del_snat(char *ip_addr, int mark);

#endif
