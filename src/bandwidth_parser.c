#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bandwidth_parser.h"
#include "debug.h"

static const char * units[__UNIT_MAX] = {
    [UNIT_UNSPEC] = "",
    [UNIT_BITS] = "b",
    [UNIT_KBITS] = "kb",
    [UNIT_MBITS] = "mb",
    [UNIT_GBITS] = "gb",
};

int find_unit_id(char *unit)
{
    int i = 0;
    for(; i < __UNIT_MAX; i++){
        if(!strcmp(units[i], unit)){
            return i;
        }
    }
    return -1;
}

int validate_string(char *value, char *r){
    regex_t regex;
    int ret;

    //[1-9][0-9]*(b|Kb|Mb|Gb)
    ret = regcomp(&regex, r, REG_EXTENDED | REG_ICASE);
    if (ret) {
        print_error("Failed to compile regex.\n");
        return -1;
    }
    ret = regexec(&regex, value, 0, 0, 0);
    if (ret == REG_NOMATCH) {
        print_error("Value string doesn't match.\n");
    } else if (ret < 0) {
        print_error("Failed to execute regex.\n");
    }
    regfree(&regex);
    return ret;
}

char *get_regex_match(char *value, char *r)
{
    static char result[16];
    char *match = (char*)0;
    regex_t regex;
    int ret = 0;
    const int n_matches = 1;
    const char * p = value;
    regmatch_t m[n_matches];

    memset(result, 0, 16);

    ret = regcomp(&regex, r, REG_EXTENDED | REG_ICASE);
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

    match = malloc(m[0].rm_eo - m[0].rm_so + 1);
    memset(match, 0, m[0].rm_eo - m[0].rm_so + 1);
    strncpy(result, value + m[0].rm_so, m[0].rm_eo - m[0].rm_so);

    regfree(&regex);
    free(match);

    return result;
}

int validate_bandwidth_value_abing(char* value)
{
    return validate_string(value, "(ABW:)\\s+[[:digit:]]+\\.[[:digit:]]+ ([KMG]{0,1}bps)");
}


int validate_bandwidth_value(char* value)
{
    return validate_string(value, "^[[:digit:]]+([KMG]{0,1}b)$");
}

char * get_bandwidth_unit_abing(char *value)
{
    return get_regex_match(value, "([KMG]{0,1}bps)");
}

char * get_bandwidth_unit(char *value)
{
    return get_regex_match(value, "([KMG]{0,1}b)$");
}

uint32_t get_bandwidth(char *value)
{
    char *match = (char*)0;
    int bw = 0;

    match = get_regex_match(value, "^[[:digit:]]+");
    bw = atoi(match);

    return bw;
}

char * get_bandwidth_value_abing(char *value)
{
    char *match = (char*)0;

    match = get_regex_match(value, "(ABW:)\\s+[[:digit:]]+\\.[[:digit:]]+ ([KMG]{0,1}bps)");

    return match;
}

double get_bandwidth_abing(char *value)
{
    char *match = (char*)0;
    double bw = 0;

    match = get_regex_match(value, "[[:digit:]]+\\.[[:digit:]]+");
    bw = atof(match);
    return bw;
}
