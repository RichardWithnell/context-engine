#include "modem_coms.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>


struct stat statf;


int get_modem_csq(int fd)
{
    char* read_buffer = malloc(512);

    char* rssi_str;
    #ifdef DEBUG
    printf("Get modem csq\n");
    #endif
    memset(read_buffer, 0 , 512); //Initialise buffer

    if(modem_command(fd, "AT+CSQ\r", read_buffer)){
        printf("CSQ Error...\n");
        return -2;
    }
    if ((rssi_str = strstr(read_buffer, "+CSQ:")) != NULL) {
        rssi_str = strtok(rssi_str, "+CSQ:");
        rssi_str = strtok(rssi_str, ",");

        /* Trim whitespace from in front of RSSI value read in */
        int i = 0;
        int dest = 0;
        char *dst = malloc(sizeof(rssi_str));
		int ret = 0;
        while(rssi_str[i]) {
            if(!isspace(rssi_str[i])) dst[dest++] = rssi_str[i];
            i++;
        }
        dst[dest++] = 0;

        free(read_buffer);

        ret = atoi(dst);
        free(dst);
        return ret;
    }

    free(read_buffer);
    #ifdef DEBUG
    printf("Read CSQ failed\n");
    #endif
    return -1;
}


int modem_command(int fd, char *in, char *out)
{
    write_line(fd, in);
    if(read_response(fd, out) != SUCCESS) return ERR;
    else return SUCCESS;
}


int read_response(int fd, char *out)
{
    char *buffer = malloc(512);
    char *temp;
    int read_times = 0;

    memset(buffer, '\0', 512);
    int i = 0;

    do{
        if(i == 200){
            return ERR;
        }

        if((temp = read_line(fd)) == NULL){
            return ERR;
        }

        if(strstr(temp, "OK")){
            /*
            printf("Buffer: %s\n", buffer);
            */
            strcpy(out, buffer);
            free(buffer);
            return SUCCESS;
        } else if(strstr(temp, "ERROR")){
            free(buffer);
            return ERR;
        }

        if(strlen(temp) > 0 && temp[0] != '^'){
            #ifdef DEBUG
            printf("%s\n", temp);
            #endif
            strcat(buffer, temp);
        }
        i++;
    } while(TRUE);
}


char* read_line(int fd)
{
    char *buffer = 0;
    char *buffptr = 0;
    int nbytes = 0;

    buffer = malloc(512);
    memset(buffer, 0, 512);

    buffptr = buffer;
    while ((nbytes = read(fd, buffptr, buffer + sizeof(buffer) - buffptr - 1)) > 0){
        buffptr += nbytes;
        if (buffptr[-1] == '\n' || buffptr[-1] == '\r'){
            break;
        }
    }

    if (strncmp(buffer, "OK", 2) == 0){
        #ifdef DEBUG
        printf("Read: %s\n", buffer);
        #endif
        return buffer;
    } else {
        return (char*)0;
    }
}


int read_timeout(int fd, char* buff, int len)
{
    fd_set set;
    struct timeval timeout;
    int rv;

    FD_ZERO(&set);
    FD_SET(fd, &set);

    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    rv = select(fd + 1, &set, NULL, NULL, &timeout);
    if(rv == -1) {
        #ifdef DEBUG
        printf("Read select error\n");
        #endif
        return ERR;
    } else if(rv == 0) {
        #ifdef DEBUG
        printf("Read timeout\n");
        #endif
        return ERR_TIMEOUT;
    } else {
        return read(fd, buff, len);
    }
}


int write_line(int fd, char *line)
{

    if(write(fd, line, strlen(line)) < 0){
        printf("Failed to write to modem\n");
        return ERR;
    } else {
        #ifdef DEBUG
        printf("Wrote: %s\n", line);
        #endif
        return SUCCESS;
    }
}


int open_modem(char* dev)
{
    int fd;
    struct termios tty;

    /* Check if the modem device is present */
    if((stat(dev, &statf) == -1 && errno == ENOENT)) {
        printf("Modem device: %s not present\n", dev);
        return -2;
    }

    fd = open(dev, O_RDWR | O_NOCTTY);

    if(fd < 0){
        return ERR;
    }

    tcgetattr(fd, &tty);

    tty.c_cflag     &=  ~PARENB;        // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_lflag = ICANON;
    tty.c_cc[VTIME] = 50;
    tty.c_cflag     |=  CREAD | CLOCAL;
    tcsetattr(fd, TCSANOW, &tty);

    #ifdef DEBUG
    printf("Opened modem\n");
    #endif
    return fd;
}


int open_modem_usb(int ttyusb)
{
    char* dev_path = malloc(16);
    int fd;

    sprintf(dev_path, "/dev/ttyUSB%d", ttyusb);
    fd = open_modem(dev_path);
    free(dev_path);
    return fd;
}


int close_modem(int fd)
{
    close(fd);
    #ifdef DEBUG
    printf("Closed modem\n");
    #endif
}
