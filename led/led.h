#include <gpiod.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <mosquitto.h>
#include <libgen.h>

/*
 * RGB LED   <----> PhyPin <----> BCM Pin
 * R         <---->  #33   <----> GPIO13
 * G         <---->  #35   <----> GPIO19
 * B         <---->  #37   <----> GPIO26
 * GND       <---->  #39   <----> GND
 * */

/* RGB blink sleep time*/
#define BLINK_T  1
#define KEEP_ALIVE 40

#ifndef CONSUMER
#define CONSUMER "Consumer"
#endif

/* RGB led index*/
enum
{
	LED_R,
	LED_G,
	LED_B,
	LED_MAX,
};

/* RGB led status */
enum
{
	OFF=0,
	ON,
};

/* RGB led struct */
typedef struct led_gpio_s
{
	int                idx;   // led index
	int                gpio; // led GPIO port
	const char        *desc;  // led description
	struct gpiod_line *line;  // led gpio line
}led_gpio_t;

/* RGB led information */
led_gpio_t leds[LED_MAX] = 
{
	{ LED_R, 13, "red", NULL},
	{ LED_G, 19, "green", NULL},
	{ LED_R, 26, "blue", NULL},
};

int g_stop = 0;

void sig_handler(int signum);

/* turn RGB led on/off API function */
void turn_led(int which, int status);

/* blink RGB led API function */
void blink_led(int which);

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

