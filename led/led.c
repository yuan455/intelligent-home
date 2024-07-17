#include "led.h"

void sig_handler(int signum)
{
	switch(signum)
	{
		case SIGINT:
		case SIGTERM:
			g_stop = 1;
		default:
			break;
	}
	return;
}

/* turn RGB led on/off API function */
void turn_led(int which, int status)
{
	int ret;
	
	if( which<0 || which>= LED_MAX )
	{
		return;
	}
	printf("turn %s led GPIO#%d %s\n", leds[which].desc, leds[which].gpio, status?"on":"off");
	
	if( ON == status )
	{
		ret = gpiod_line_set_value(leds[which].line, 1);
		if( ret < 0 )
		{
			printf("Set line[%d] output falied. val:%d. reason: %s\n", leds[which].gpio, status, strerror(errno));
			return ;
		}
	}
	else if( OFF == status )
	{
		ret = gpiod_line_set_value(leds[which].line, 0);
		if( ret < 0 )
		{
			printf("Set line[%d] output falied. val:%d. reason: %s\n", leds[which].gpio, status, strerror(errno));
		        return ;
		}

	}
}

/* blink RGB led API function */
void blink_led(int which)
{
	if( which<0 || which>= LED_MAX )
		return;

	turn_led(which, ON);
	sleep(BLINK_T);

	turn_led(which, OFF);
	sleep(BLINK_T);
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
	printf("Receive a message of %s : %s\n", (char *)msg->topic, (char *)msg->payload);
	if(strstr(msg->payload, "RedLed"))
	{
		if(strstr(msg->payload, "on"))
			turn_led(LED_R, ON);
		else if(strstr(msg->payload, "off"))
			turn_led(LED_R, OFF);
	}
	else if(strstr(msg->payload, "GreenLed"))
	{
		if(strstr(msg->payload, "on"))
			turn_led(LED_G, ON);
		else if(strstr(msg->payload, "off"))
			turn_led(LED_G, OFF);
	}
	else if(strstr(msg->payload, "BlueLed"))
	{
		if(strstr(msg->payload, "on"))
			turn_led(LED_B, ON);
		else if(strstr(msg->payload, "off"))
			turn_led(LED_B, OFF);
	}
}


