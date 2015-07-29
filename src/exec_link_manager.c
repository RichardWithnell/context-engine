#include "util.h"
#include "exec_link_manager.h"
#include "debug.h"

enum {
    LINK_UNSPEC,
    LINK_UP,
    LINK_DOWN
};


int exec_link_manager(char * ifname, int action)
{
    FILE* output = (FILE*)0;
    int status = 0;
    int pid = 0;
    char line[512];
    char *cmd = (char*)0;
    int ret_code = 0;

    if(!ifname){
        print_error("Interface Name Is null\n");
        return -1;
    }

    if(action == LINK_UP){
        cmd = malloc(32);
        if(!cmd){
            return -1;
        }
        strcpy(cmd, "");
        strcat(cmd, "ifup ");
        strcat(cmd, ifname);
        print_verb("CMD: %s\n", cmd);
        output = execv_and_pipe("/sbin/ifup", cmd, &pid);
    } else if (action == LINK_DOWN) {
        cmd = malloc(32);
        if(!cmd){
            return -1;
        }
        strcpy(cmd, "");
        strcat(cmd, "ifdown ");
        strcat(cmd, ifname);
        print_verb("CMD: %s\n", cmd);
        output = execv_and_pipe("/sbin/ifdown", cmd, &pid);
    } else {
        free(cmd);
        return -1;
    }

    if(!output){
        free(cmd);
        return -1;
    }

    while(fgets(line, sizeof(line), output)) {
        print_verb("ifupdown: %s\n", line);
    }
    ret_code = waitpid(pid, &status, 0);

    free(cmd);

    return ret_code;
}

int exec_link_manager_down(char *ifname)
{
    return exec_link_manager(ifname, LINK_DOWN);
}

int exec_link_manager_up(char *ifname)
{
    return exec_link_manager(ifname, LINK_UP);
}
