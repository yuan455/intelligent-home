#include "sht20.c"

int main(int argc, char **argv)
{
	int     fd;
	float   temp;
	float   humi;
	uint8_t serialnumber[8];
	int     ch;
	char    *send_data;
	int     port = 0;
	char    *serv_name = NULL;
	char    *serv_ip = NULL;
	char    *topic = NULL;
	char    *dev_i2c = NULL;
	char             *program_name;
	int     mid;
	struct mosquitto *mosq = NULL;
	bool              session = true;
	int               daemon_run = 0;
	int               connect = 0;
	
	struct option opts[] = {
		{"i2c", required_argument, NULL, 'i'},
		{"host", required_argument, NULL, 'h'},
		{"port", required_argument, NULL, 'p'},
		{"topic", required_argument, NULL, 't'},
		{"daemon", no_argument, NULL, 'd'},
		{NULL, 0, NULL, 0}
	};

	program_name = basename(argv[0]);

	while( (ch = getopt_long(argc, argv, "i:h:p:t:", opts, NULL)) != -1)
	{
		switch(ch)
		{
			case 'i':
				dev_i2c = optarg;
				break;
			case 'p':
				port = atoi(optarg);
				break;
			case 'h':
				serv_name = optarg;
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

	if( !serv_name || !port || !dev_i2c || !topic)
	{
		printf("%s usage: \n", program_name);
		printf("--i2c(-i) i2c device name\n");
		printf("--host(-h) MQTT broker IP\n");
		printf("--port(-p) MQTT broker port\n");
		printf("--topic(-t) MQTT broker topic\n");
		printf("--daemon(-d) set program running on background\n");
		printf("For example: ./%s -i /dev/i2c-1 -h localhost -p 1883 -t test -d\n", program_name);
		return -1;
	}

	// 安装信号
	signal(SIGINT, signal_stop);   // ctrl+c
	signal(SIGTERM, signal_stop);  // kill

	// 后台运行
	if( daemon_run )
	{
		daemon(0, 0);
	}

	// 域名解析
	struct hostent *serv_host = gethostbyname(serv_name);
	if(serv_host == NULL)
	{
		printf("get host by name failed: %s\n", strerror(errno));
		return -2;
	}
	serv_ip = inet_ntoa(*(struct in_addr *)serv_host->h_addr);

	fd = sht2x_init(dev_i2c);
	if( fd < 0 )
	{
		printf("SHT2x initialize failure\n");
		return -1;
	}

	if(sht2x_get_serialnumber(fd, serialnumber, 8) < 0)
	{
		printf("SHT2x get serial number failure\n");
		return -2;
	}

	while(g_stop)
	{
		// 获取温湿度
		if(sht2x_get_temp_humi(fd, &temp, &humi) < 0 )
		{
			printf("SHT2x get temperature and relative humidity failure\n");
			return -3;
		}

		//printf("Temperature=%lf ℃  relative humidity=%lf%\n", temp, humi);
		
		// 将温湿度打包成JSON格式
		send_data = (char *)malloc(128);
		snprintf(send_data, 128, "{\"Temperature\":\"%.2f\", \"Humidity\":\"%.2f\"}", temp, humi);
		//printf("strlen(send_data)=%d send_data=%s\n", strlen(send_data), send_data);

		// 通过MQTT上传
		// 如果没有连接MQTT，就使用connect连接MQTT
		if(!connect)
		{
			mosquitto_lib_init();
			mosq = mosquitto_new(NULL, session, NULL);
			if(!mosq)
			{
				printf("Mosquitto_new() failed: %s\n", strerror(errno));
				mosquitto_destroy(mosq);
				mosquitto_lib_cleanup();
				continue;
			}
			printf("Create mosquitto successfully\n");

			if(mosquitto_connect(mosq, serv_ip, port, KEEP_ALIVE) != MOSQ_ERR_SUCCESS)
			{
				printf("Mosq_connect() failed: %s\n", strerror(errno));
				mosquitto_destroy(mosq);
				mosquitto_lib_cleanup();
				continue;

			}
			printf("connect %s:%d successfully\n", serv_ip, port);

			int loop = mosquitto_loop_start(mosq);
			if(loop != MOSQ_ERR_SUCCESS)
			{
				printf("mosquitto loop failed:%s\n", strerror(errno));
				mosquitto_destroy(mosq);
				mosquitto_lib_cleanup();
				continue;
			}
			connect = 1;
		}

		if(mosquitto_publish(mosq, &mid, topic, strlen(send_data)+1, send_data, 0, 0) != MOSQ_ERR_SUCCESS)
		{
			printf("Mosq_publish() failed: %s\n", strerror(errno));
			mosquitto_destroy(mosq);
			mosquitto_lib_cleanup();
			connect = 0;
			continue;

		}
		printf("publish %s to [%s:%d] successfully\n", send_data, serv_ip, port);
		sleep(30);
	}	
Cleanup:
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	close(fd);
	free(send_data);
	return 0;
}

