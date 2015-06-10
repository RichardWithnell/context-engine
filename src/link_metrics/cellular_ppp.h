#ifndef CONTEXT_CELLULAR_PPP
#define CONTEXT_CELLULAR_PPP

char* read_line(int fd);
int write_line(int fd, char *line);
int open_modem(char* dev);
int close_modem(int fd);
int read_response(int fd, char *out);
int modem_command(int fd, char *in, char *out);
int open_modem_usb(int ttyusb);
int read_timeout(int fd, char* buff, int len);
int get_modem_csq(int fd);

#define TRUE 1
#define FALSE 0

#define SUCCESS 0x00
#define ERR -1
#define ERR_TIMEOUT -2

#endif
