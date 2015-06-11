#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/iptables/iptables.h"
#include "../src/flow_manager/route_enforcer.h"
#include "../src/resource_manager.h"
#include "../src/application_rules.h"


int main(void)
{
    init_iptables();

    return 0;
}
