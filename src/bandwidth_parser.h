#ifndef  BANDWIDTH_PARSER
#define BANDWIDTH_PARSER

#include <stdint.h>

enum {
    UNIT_UNSPEC = 0x00,
    UNIT_BITS = 0x01,
    UNIT_KBITS = 0x02,
    UNIT_MBITS = 0x03,
    UNIT_GBITS = 0x04,
    __UNIT_MAX = 0x05,
};

#define GB_IN_B 1073741824
#define MB_IN_B 1048576
#define KB_IN_B 1024


#define KB_TO_B(x) (x/KB_IN_B)
#define MB_TO_B(x)  (x/MB_IN_B)
#define GB_TO_B(x)  (x/GB_IN_B)

int find_unit_id(char *unit);

int validate_bandwidth_value(char* value);
char * get_bandwidth_unit(char *value);
uint32_t get_bandwidth(char *value);

int validate_bandwidth_value_abing(char* value);
char * get_bandwidth_unit_abing(char *value);
double get_bandwidth_abing(char *value);
char * get_bandwidth_value_abing(char *value);
#endif
