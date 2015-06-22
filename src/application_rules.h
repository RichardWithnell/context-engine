#ifndef CONTEXT_APPLICATION_RULE
#define CONTEXT_APPLICATION_RULE

#include "list.h"

enum {
    TCP = 6,
    UDP = 17,
};

enum {
    RULE_MULTIPATH_UNSPEC,
    RULE_MULTIPATH_ENABLED,
    RULE_MULTIPATH_DISABLED,
    RULE_MULTIPATH_HANDOVER,
};

enum {
    RULE_ALLOCATE_RESOURCE_UNSPEC,
    RULE_ALLOCATE_RESOURCE_YES,
    RULE_ALLOCATE_RESOURCE_NO,
};

#define DEFAULT_MULTIPATH_BEHAVIOUR RULE_MULTIPATH_ENABLED

struct application_spec;
struct rule_spec;

List * load_application_specs(char *config_file);

/*Memory*/
struct application_spec *application_spec_alloc(void);
void application_spec_free(struct application_spec *as);

/*Setters*/
void application_spec_set_daddr(struct application_spec *as, uint32_t daddr);
void application_spec_set_saddr(struct application_spec *as, uint32_t saddr);
void application_spec_set_dport(struct application_spec *as, uint16_t dport);
void application_spec_set_sport(struct application_spec *as, uint16_t sport);
void application_spec_set_proto(struct application_spec *as, uint8_t proto);
void application_spec_set_required_bw(struct application_spec *as, uint32_t bw);
void application_spec_set_required_latency(struct application_spec *as, double latency);
void application_spec_set_required_loss(struct application_spec *as, double loss);
void application_spec_set_required_loss(struct application_spec *as, double jitter);
void application_spec_set_multipath(struct application_spec *as, uint8_t mp);

/*Getters*/
uint32_t application_spec_get_daddr(struct application_spec *as);
uint32_t application_spec_get_saddr(struct application_spec *as);
uint16_t application_spec_get_dport(struct application_spec *as);
uint16_t application_spec_get_sport(struct application_spec *as);
uint8_t application_spec_get_proto(struct application_spec *as);
uint32_t application_spec_get_required_bw(struct application_spec *as);
double application_spec_get_required_latency(struct application_spec *as);
double application_spec_get_required_loss(struct application_spec *as);
double application_spec_get_required_jitter(struct application_spec *as);
uint8_t application_spec_get_multipath(struct application_spec *as);

#endif
