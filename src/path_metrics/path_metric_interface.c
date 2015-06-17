#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "path_metric_interface.h"
#include "../debug.h"

struct path_stats {
    double delay;
    double loss;
    double jitter;
    uint32_t bandwidth;
};

void print_path_stats(struct path_stats *stats, char *id)
{
    print_debug("%s - Bandwidth: %zu Delay: %f Loss: %f Jitter: %f\n",
        id, stats->bandwidth, stats->delay, stats->loss, stats->jitter);
}

double metric_cmp(struct path_stats *ps1, struct path_stats *ps2)
{
    double diff = 0;

    if(ps1->delay != ps2->delay){
        diff += fdim(ps1->delay, ps2->delay) / fmax(ps1->delay, ps2->delay);
    }

    if(ps1->loss != ps2->loss){
        diff += fdim(ps1->loss, ps2->loss) / fmax(ps1->loss, ps2->loss);
    }

    if(ps1->jitter != ps2->jitter){
        diff += fdim(ps1->jitter, ps2->jitter) / fmax(ps1->jitter, ps2->jitter);
    }

    if(ps1->bandwidth != ps2->bandwidth){
        diff += fdim(ps1->bandwidth, ps2->bandwidth)
                / fmax(ps1->bandwidth, ps2->bandwidth);
    }

    return diff;
}

int metric_update(struct path_stats *s, char *server, char *local)
{
    print_debug("Update Metrics\n");

    if(!s){
        print_error("Couldnt update metric, path_stats null\n");
        return -1;
    }

    if(!server){
        print_error("Couldnt update metric, server null\n");
        return -1;
    }

    if(!local){
        print_error("Couldnt update metric, local null\n");
        return -1;
    }

    return populate_path_stats(s, server, local);
}

struct path_stats *metric_alloc(void)
{
    struct path_stats *ps;
    ps = malloc(sizeof(struct path_stats));
    return ps;
}

struct path_stats *metric_clone(struct path_stats *stats)
{
    struct path_stats *ps;
    ps = malloc(sizeof(struct path_stats));
    memcpy(ps, stats, sizeof(struct path_stats));
    return ps;
}

void metric_free(struct path_stats *ps)
{
    free(ps);
}

/*Setters*/
void metric_set_bandwidth(struct path_stats *s, uint32_t bandwidth)
{
    s->bandwidth = bandwidth;
}

void metric_set_delay(struct path_stats *s, double delay)
{
    s->delay = delay;
}

void metric_set_jitter(struct path_stats *s, double jitter)
{
    s->jitter = jitter;
}

void metric_set_loss(struct path_stats *s, double loss)
{
    s->loss = loss;
}

/*Getters*/
uint32_t metric_get_bandwidth(struct path_stats *s)
{
    return s->bandwidth;
}

double metric_get_delay(struct path_stats *s)
{
    return s->delay;
}

double metric_get_jitter(struct path_stats *s)
{
    return s->jitter;
}

double metric_get_loss(struct path_stats *s)
{
    return s->loss;
}
