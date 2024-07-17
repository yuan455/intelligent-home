#include "led.c"
int main(int argc, char **argv)
{
	struct gpiod_line      *line;
	int                     ret;
	const char             *chipname = "gpiochip0";
	struct gpiod_chip      *chip;
	int                     i, j;
	int                     rv = 0;
	char                   *serv_name = NULL;
	char                   *serv_ip = NULL;
	char                   *topic = NULL;
	struct mosquitto       *mosq;
	int                     daemon_run = 0;
	bool                    session = true;
	int                     port = 0;
	char                   *program_name = NULL;
	int                     ch;

	struct option           opts[] = {
		{"host", required_argument, NULL, 'h'},
		{"port", required_argument, NULL, 'p'},
		{"topic", required_argument, NULL, 't'},
		{"daemon", no_argument, NULL, 'd'},
		{NULL, 0, NULL, 0}
	};
	program_name = basename(argv[0]);

	while((ch = getopt_long(argc, argv, "h:p:t:d", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'h':
				serv_name = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 't':
				topic = optarg;
				break;
			case 'd':
				daemon_run = 1;
				break;
			default:
				break;
		}
	}

	if( !serv_name || !port || !topic )
	{
		printf("%s Usage:\n", program_name);
		printf("--host(-h) MQTT broker IP\n");
		printf("--port(-p) MQTT broker port\n");
		printf("--topic(-t) MQTT broker topic\n");
		printf("--daemon(-d) set program running on background\n");
		printf("For example: ./%s -h localhost -p 1883 -t test\n", program_name);
		return -1;
	}

	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	if(daemon_run)
	{
		daemon(0, 0);
	}

	struct hostent *serv_host = gethostbyname(serv_name);
	if(serv_host == NULL)
	{
		printf("get host by name failed: %s\n", strerror(errno));
		return -2;
	}
	serv_ip = inet_ntoa(*(struct in_addr*)serv_host->h_addr);

	/* open GPIO chip */
	chip = gpiod_chip_open_by_name(chipname);
	if( !chip )
	{
		printf("gpiod open '%s' failed: %s\n", chipname, strerror(errno));
		return -1;
	}
	
	for( i=0; i<LED_MAX; i++ )
	{
		leds[i].line = gpiod_chip_get_line(chip, leds[i].gpio);
		if( !leds[i].line )
		{
			printf("Get line failed: %s\n", strerror(errno));
			gpiod_line_release(leds[i].line);
			rv = -1;
			goto cleanup;
		}

		ret = gpiod_line_request_output(leds[i].line, CONSUMER, 0);
		if( ret < 0 )
		{
			printf("Request line[%d] as output falied: %s\n", leds[i].gpio, strerror(errno));
			gpiod_line_release(leds[i].line);
			rv = -1;
			goto cleanup;
		}
	}

	// 连接mosquitton
	rv = mosquitto_lib_init();
	if(rv)
	{
		printf("Init lib failed:%s\n", strerror(errno));
		return -3;
	}

	mosq = mosquitto_new(NULL, session, NULL);
	if(!mosq)
	{
		printf("Mosquitto_new() failed:%s\n", strerror(errno));
		mosquitto_lib_cleanup();
		return -1;
	}

	mosquitto_message_callback_set(mosq, my_message_callback);

	rv = mosquitto_connect(mosq, serv_ip, port, KEEP_ALIVE);
	if(rv)
	{
		printf("Mosq_connect() failed:%s\n", strerror(errno));
		mosquitto_destroy(mosq);
		mosquitto_lib_cleanup();
		return -1;
	}

	if( mosquitto_subscribe(mosq, NULL, topic, 2 ))
	{
		printf("Mosq_subscribe failed:%s\n", strerror(errno));
		return -1;
	}

	while( !g_stop )
	{
		mosquitto_loop(mosq, -1, 1);
	}

	/* turn off leds and release lines*/
	for( j=0; j<LED_MAX; j++ )
	{
		turn_led(j, OFF);
		gpiod_line_release(leds[j].line);
	}

	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	/* release chip */
cleanup:
	gpiod_chip_close(chip);
	return rv;
}
