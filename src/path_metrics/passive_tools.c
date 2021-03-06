#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "path_metric_interface.h"
#include "../debug.h"

double get_loss_stat(char *value);
int get_ping_stats(char *host, char *bind, struct path_stats *stats);
int get_iperf_bandwidth(char *host, char *localaddr, struct path_stats *stats);

int populate_path_stats(struct path_stats *ps, char *server, char *local)
{
    if(get_iperf_bandwidth(server, local, ps)){
        print_error("Failed to get bandwidth\n");
        return -1;
    }
    if(get_ping_stats(server, local, ps)){
        print_error("Failed to get ping stats\n");
        return -1;
    }
    return 0;
}

double get_loss_stat(char *value)
{
    char *match = (char*)0;
    char *pch, *ptr;
    regex_t regex;
    int ret = 0;
    const int n_matches = 1;
    const char * p = value;
    regmatch_t m[n_matches];
    double loss_stat = 0.00;

    ret = regcomp(&regex, "[[:digit:]]+% packet loss", REG_EXTENDED | REG_ICASE);
    if (ret) {
        print_error("Failed to compile regex.\n");
        return -1;
    }

    ret = regexec(&regex, p, n_matches, m, 0);
    if (ret) {
        print_error("Regex Match Not Found...\n");
        return -1;
    }

    if (m[0].rm_so == -1) {
        print_error("Regex Match rm.so == -1\n");
        return -1;
    }

    match = malloc(m[0].rm_eo - m[0].rm_so + 1);
    memset(match, 0, m[0].rm_eo - m[0].rm_so + 1);
    strncpy(match, value + m[0].rm_so, m[0].rm_eo - m[0].rm_so);

    pch = strtok(match, "%");
    loss_stat = strtof(pch, &ptr);

    regfree(&regex);
    free(match);

    return loss_stat;
}

int get_ping_stats(char *host, char *bind, struct path_stats *stats)
{
    pid_t pid = 0;
    int pipefd[2];
    FILE* output;
    char line[256];
    int status;

    pipe(pipefd); //create a pipe
    pid = fork(); //span a child process
    if (pid == 0) {
        // Child. Let's redirect its standard output to our pipe and replace process with tail
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        execl("/bin/ping", "/bin/ping", "-I", bind, "-c", "5", "-q", host, (char*)NULL);
    }

    //Only parent gets here. Listen to what the tail says
    close(pipefd[1]);
    output = fdopen(pipefd[0], "r");

    //listen to what tail writes to its standard output
    while(fgets(line, sizeof(line), output)) {
        

        if(strstr(line, "error")){
            print_error("ping failed\n");
            metric_set_jitter(stats, -1.00);
            metric_set_loss(stats, -1.00);
            metric_set_delay(stats, -1.00);
            return -1;
        } else if(strstr(line, "loss")){
            metric_set_loss(stats, get_loss_stat(line));
        } else if(strstr(line, "rtt")){
            char *pch, *pch2, *ptr;
            pch = strtok (line, "=");
            pch = strtok (NULL, "=");

            pch2 = strtok (pch, "/ ");
            pch2 = strtok (NULL, "/ ");
            metric_set_delay(stats, strtof(pch2, &ptr));
            pch2 = strtok (NULL, "/ ");
            pch2 = strtok (NULL, "/ ");
            metric_set_jitter(stats, strtof(pch2, &ptr));
        }
    }
    waitpid(pid, &status, 0);

    print_verb("Latency: %f Jitter: %f Loss: %f\n",
        metric_get_delay(stats),
        metric_get_jitter(stats),
        metric_get_loss(stats));


    return 0;
}

int get_iperf_bandwidth(char *host, char *localaddr, struct path_stats *stats)
{
    pid_t pid = 0;
    int pipefd[2];
    FILE* output;
    char line[256];
    int status;
    uint32_t bandwidth = 0;

    pipe(pipefd); //create a pipe
    pid = fork(); //span a child process
    if (pid == 0) {
        // Child. Let's redirect its standard output to our pipe and replace process with tail
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        execl("/usr/bin/iperf", "/usr/bin/iperf", "-c", host, "-y", "C", (char*)NULL);
    }

    //Only parent gets here. Listen to what the tail says
    close(pipefd[1]);
    output = fdopen(pipefd[0], "r");

    //listen to what tail writes to its standard output
    while(fgets(line, sizeof(line), output)) {
        int i = 0;
        char *pch, *ptr;

        if(strstr(line, "failed") || strstr(line, "refused")){
            waitpid(pid, &status, 0);
            return -1;
        }

        print_verb("iperf out: %s\n", line);
        pch = strtok (line, ",");
        while(i++ < 8){
            pch = strtok (NULL, ",");
        }
        bandwidth = strtol(pch, &ptr, 10);
    }
    waitpid(pid, &status, 0);

    metric_set_bandwidth(stats, bandwidth);

    return 0;
}
