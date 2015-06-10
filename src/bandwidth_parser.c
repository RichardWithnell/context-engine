#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bandwidth_parser.h"
#include "debug.h"

int validate_bandwidth_value(char* value)
{
    regex_t regex;
    int ret;

    //[1-9][0-9]*(b|Kb|Mb|Gb)
    ret = regcomp(&regex, "^[[:digit:]]+([KMG]{0,1}b)$", REG_EXTENDED | REG_ICASE);
    if (ret) {
        print_error("Failed to compile regex.\n");
        return -1;
    }
    print_debug("Regexec on: %s\n", value);
    ret = regexec(&regex, value, 0, 0, 0);
    if (!ret) {
        print_verb("Regex Match Found.\n");
    } else if (ret == REG_NOMATCH) {
        print_error("Value string doesn't match.\n");
    } else {
        print_error("Failed to execute regex.\n");
    }
    regfree(&regex);
    return ret;
}

char * get_bandwidth_unit(char *value)
{
    static char result[16];
    regex_t regex;
    int ret = 0;
    const int n_matches = 1;
    const char * p = value;
    regmatch_t m[n_matches];

    memset(result, 0, 16);

    ret = regcomp(&regex, "([KMG]{0,1}b)$", REG_EXTENDED | REG_ICASE);
    if (ret) {
        print_error("Failed to compile regex.\n");
        return (char*)0;
    }

    ret = regexec(&regex, p, n_matches, m, 0);
    if (ret) {
        print_error("Regex Match Not Found...\n");
        return (char*)0;
    }

    if (m[0].rm_so == -1) {
        print_error("Regex Match rm.so == -1\n");
        return (char*)0;
    }

    strncpy(result, value + m[0].rm_so, m[0].rm_eo - m[0].rm_so);

    regfree(&regex);

    return result;
}

uint32_t get_bandwidth(char *value)
{
    char *match = (char*)0;
    regex_t regex;
    int bw = 0;
    int ret = 0;
    const int n_matches = 1;
    const char * p = value;
    regmatch_t m[n_matches];

    ret = regcomp(&regex, "^[[:digit:]]+", REG_EXTENDED | REG_ICASE);
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
    print_debug("Match Unit: %s\n", match);

    bw = atoi(match);

    regfree(&regex);
    free(match);

    return bw;
}
