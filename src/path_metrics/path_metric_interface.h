#ifndef CONTEXT_PATH_METRIC
#define CONTEXT_PATH_METRIC

#include <stdint.h>

struct path_stats;

int metric_update(struct path_stats *s, char *server, char *local);
int populate_path_stats(struct path_stats *ps, char *server, char *local);
void print_path_stats(struct path_stats *stats, char *id);

int metric_cmp(struct path_stats *ps1, struct path_stats *ps2);

/*Memory*/
struct path_stats *metric_alloc(void);
struct path_stats *metric_clone(struct path_stats *stats);
void metric_free(struct path_stats *ps);

/*Setters*/
void metric_set_bandwidth(struct path_stats *s, uint32_t bandwidth);
void metric_set_delay(struct path_stats *s, double delay);
void metric_set_jitter(struct path_stats *s, double jitter);
void metric_set_loss(struct path_stats *s, double loss);

/*Getters*/
uint32_t metric_get_bandwidth(struct path_stats *s);
double metric_get_delay(struct path_stats *s);
double metric_get_jitter(struct path_stats *s);
double metric_get_loss(struct path_stats *s);

#endif
