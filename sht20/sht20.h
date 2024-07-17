#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <mosquitto.h>
#include <getopt.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <libgen.h> // for basename
#include <signal.h>

#define SOFTRESET             0xFE
#define TRIGGER_TEMP_NO_HOLD  0xF3
#define TRIGGER_HUMI_NO_HOLD  0xF5
#define KEEP_ALIVE            60

static inline void msleep(unsigned long ms);
static inline void dump_buf(const char *prompt, uint8_t *buf, int size);
int sht2x_init(char *i2c_dev);
int sht2x_get_serialnumber(int fd, uint8_t *serialnumber, int size);
int sht2x_get_temp_humi(int fd, float *temp, float *rh);
void signal_stop(int signum); 

int               g_stop = 1;

