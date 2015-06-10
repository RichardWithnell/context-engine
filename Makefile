#
# Should probably move to autoconf soon
#

LIB_PATH = /usr/lib/
INC_PATH = /usr/include/libnl3
LDFLAGS = -lm -lnl-3 -lnl-route-3 -lnl-genl-3 -lrt -lmnl -lpthread -lnetfilter_conntrack -ldl
CC=gcc
CFLAGS= -g -Wall

ifndef ARCH
	ARCH:=$(shell uname -m)
endif

ifeq ($(ARCH),sim)
    CFLAGS += -DDCE_NS3_FIX -fPIC -U_FORTIFY_SOURCE -fstack-protector-all -Wstack-protector -fno-omit-frame-pointer
    OPTS = -pie -rdynamic
endif

ifndef METRIC_TOOLS
	METRIC_TOOLS:=standard_tools
endif

SRC_PATH=src/
BUILD_PATH=build/$(ARCH)/
BIN_PATH=bin/$(ARCH)/

OBJS =	$(BUILD_PATH)lmnl_interface.o \
		$(BUILD_PATH)list.o \
		$(BUILD_PATH)queue.o \
		$(BUILD_PATH)policy_loader.o \
		$(BUILD_PATH)link_monitor.o \
		$(BUILD_PATH)condition.o \
		$(BUILD_PATH)context_library.o \
		$(BUILD_PATH)resource_manager.o \
		$(BUILD_PATH)link_loader.o \
		$(BUILD_PATH)mptcp_controller.o \
		$(BUILD_PATH)mptcp_state.o \
		$(BUILD_PATH)path_metrics.o \
		$(BUILD_PATH)metric_tools.o \
		$(BUILD_PATH)action.o \
		$(BUILD_PATH)util.o \
		$(BUILD_PATH)bandwidth_parser.o \
		$(BUILD_PATH)iptables.o \
		$(BUILD_PATH)iptables_rule.o \
		$(BUILD_PATH)application_rules.o \
		$(BUILD_PATH)policy_handler.o \
		cjson/cJSON.o

all: context-engine

build_arch_dir:
	@if [ ! -d "$(BUILD_PATH)" ]; then mkdir -p $(BUILD_PATH); fi;

bin_arch_dir:
	@if [ ! -d "$(BIN_PATH)" ]; then mkdir -p $(BIN_PATH); fi;

context-engine: build_arch_dir bin_arch_dir $(SRC_PATH)main.c $(OBJS) cjson condition_libs
	$(CC) $(CFLAGS) -o $(BIN_PATH)context-engine $(SRC_PATH)main.c $(OPTS) $(OBJS) -I$(INC_PATH) $(LDFLAGS)

cjson:
	make -C "cjson/"

condition_libs:
	make -C "src/conditions/"

$(BUILD_PATH)condition.o: $(SRC_PATH)conditions/condition.c $(SRC_PATH)conditions/condition.h $(SRC_PATH)policy.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)conditions/condition.c  -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)condition.o

$(BUILD_PATH)policy_handler.o: $(SRC_PATH)flow_manager/policy_handler.c $(SRC_PATH)flow_manager/mptcp_state.h $(SRC_PATH)flow_manager/policy_handler.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)flow_manager/policy_handler.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)policy_handler.o

$(BUILD_PATH)mptcp_controller.o: $(SRC_PATH)flow_manager/mptcp_controller.c $(SRC_PATH)flow_manager/mptcp_controller.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)flow_manager/mptcp_controller.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)mptcp_controller.o

$(BUILD_PATH)mptcp_state.o: $(SRC_PATH)flow_manager/mptcp_state.c $(SRC_PATH)flow_manager/mptcp_state.h $(SRC_PATH)list.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)flow_manager/mptcp_state.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)mptcp_state.o

$(BUILD_PATH)iptables_rule.o: $(SRC_PATH)iptables/iptables_rule.c $(SRC_PATH)iptables/iptables.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)iptables/iptables_rule.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)iptables_rule.o

$(BUILD_PATH)iptables.o: $(SRC_PATH)iptables/iptables.c $(SRC_PATH)iptables/iptables.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)iptables/iptables.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)iptables.o

$(BUILD_PATH)metric_tools.o: $(SRC_PATH)path_metrics/$(METRIC_TOOLS).c $(SRC_PATH)path_metrics/path_metric_interface.h $(SRC_PATH)list.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)path_metrics/$(METRIC_TOOLS).c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)metric_tools.o

$(BUILD_PATH)path_metrics.o: $(SRC_PATH)path_metrics/path_metric_interface.c $(SRC_PATH)path_metrics/path_metric_interface.h $(SRC_PATH)list.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)path_metrics/path_metric_interface.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)path_metrics.o

$(BUILD_PATH)bandwidth_parser.o: $(SRC_PATH)bandwidth_parser.c $(SRC_PATH)bandwidth_parser.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)bandwidth_parser.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)bandwidth_parser.o

$(BUILD_PATH)resource_manager.o: $(SRC_PATH)resource_manager.c $(SRC_PATH)resource_manager.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)resource_manager.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)resource_manager.o

$(BUILD_PATH)link_loader.o: $(SRC_PATH)link_loader.c $(SRC_PATH)resource_manager.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)link_loader.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)link_loader.o

$(BUILD_PATH)application_rules.o: $(SRC_PATH)application_rules.c $(SRC_PATH)application_rules.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)application_rules.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)application_rules.o

$(BUILD_PATH)context_library.o: $(SRC_PATH)context_library.c $(SRC_PATH)policy.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)context_library.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)context_library.o

$(BUILD_PATH)action.o: $(SRC_PATH)action.c $(SRC_PATH)action.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)action.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -o $(BUILD_PATH)action.o

$(BUILD_PATH)link_monitor.o: $(SRC_PATH)link_monitor.c $(SRC_PATH)link_monitor.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)link_monitor.c -I$(INC_PATH) $(OPTS) $(LDFLAGS) -lmnl -o $(BUILD_PATH)link_monitor.o

$(BUILD_PATH)lmnl_interface.o: $(SRC_PATH)lmnl_interface.c $(SRC_PATH)lmnl_interface.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)lmnl_interface.c -I$(INC_PATH) $(LDFLAGS) $(OPTS) -o $(BUILD_PATH)lmnl_interface.o

$(BUILD_PATH)policy_loader.o: $(BUILD_PATH)util.o $(BUILD_PATH)list.o cjson/cJSON.o $(SRC_PATH)policy.h $(SRC_PATH)policy_loader.c
	$(CC) $(CFLAGS) -c $(SRC_PATH)policy_loader.c -o $(BUILD_PATH)policy_loader.o

$(BUILD_PATH)util.o: $(SRC_PATH)util.c $(SRC_PATH)util.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)util.c -I$(INC_PATH) $(LDFLAGS) $(OPTS) -o $(BUILD_PATH)util.o

$(BUILD_PATH)queue.o: $(SRC_PATH)queue.c $(SRC_PATH)queue.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)queue.c $(OPTS) -o $(BUILD_PATH)queue.o

$(BUILD_PATH)list.o: $(SRC_PATH)list.c $(SRC_PATH)list.h
	$(CC) $(CFLAGS) -c $(SRC_PATH)list.c $(OPTS) -o $(BUILD_PATH)list.o

clean:
	@echo "Cleaning..."
	make clean -C "cjson/"
	make clean -C "src/conditions/"
	- rm $(BUILD_PATH)* $(BIN_PATH)*
