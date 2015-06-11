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
    char line[512];
    char *cmd = (char*)0;

    cmd = malloc(strlen(command)+1);

    strcpy(cmd, command);

    output = execv_and_pipe("/usr/local/sbin/iptables", cmd, &pid);

    while(fgets(line, sizeof(line), output)) {
        print_debug("iptables: %s\n", line);
    }
    waitpid(pid, &status, 0);

    free(cmd);

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
