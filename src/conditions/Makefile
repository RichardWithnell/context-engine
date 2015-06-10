#
# Should probably move to autoconf soon
#

LIB_PATH = /usr/lib/
INC_PATH = /usr/include/libnl3
LDFLAGS = -lm -lnl-3 -lnl-route-3 -lrt -lmnl -lpthread -lnetfilter_conntrack
CC=gcc
CFLAGS= -g -Wall

all: battery bandwidth location

battery: battery_condition.c condition.c condition.h
	$(CC) $(CFLAGS) -fPIC -c battery_condition.c condition.c
	gcc -shared -Wl,-soname,libbattery_policy.so -o libbattery_policy.so battery_condition.o condition.o

bandwidth: bandwidth_condition.c condition.c ../bandwidth_parser.c condition.h ../bandwidth_parser.h
	$(CC) $(CFLAGS) -fPIC -c bandwidth_condition.c condition.c ../bandwidth_parser.c
	gcc -shared -Wl,-soname,libbandwidth_policy.so -o libbandwidth_policy.so bandwidth_condition.o bandwidth_parser.o condition.o

location: location_condition.c condition.c condition.h
	$(CC) $(CFLAGS) -fPIC -c location_condition.c condition.c
	gcc -shared -Wl,-soname,liblocation_policy.so -o liblocation_policy.so location_condition.o condition.o

clean:
	rm *.o *.so
