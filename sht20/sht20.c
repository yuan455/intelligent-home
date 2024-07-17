#include "sht20.h"

static inline void msleep(unsigned long ms)
{
	struct timespec cSleep;
	unsigned long ulTmp;

	cSleep.tv_sec = ms / 1000;
	if(cSleep.tv_sec == 0)
	{
		ulTmp = ms * 10000;
		cSleep.tv_nsec = ulTmp * 100;
	}
	else
	{
		cSleep.tv_nsec = 0;
	}

	nanosleep(&cSleep, 0);
}

static inline void dump_buf(const char *prompt, uint8_t *buf, int size)
{
	int      i;

	if( !buf || !prompt )
	{
		printf("%s Invalid parameter\n", __FUNCTION__);
		return;
	}

	if( prompt )
	{
		printf("%s ", prompt);
	}

	for(i=0; i<size; i++)
	{
		printf("%02x ", buf[i]);
	}
	printf("\n");

	return ;
}

int sht2x_softreset(int fd)
{
	uint8_t     buf[4];

	if(fd < 0)
	{
		printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__);
		return -1;
	}

	memset(buf, 0, sizeof(buf));

	buf[0] = SOFTRESET;
	write(fd, buf, 1);

	msleep(50);

	return 0;
}

int sht2x_init(char *i2c_dev)
{
	int         fd;

	if(i2c_dev == NULL)
	{
		printf("%s get invalid parameter\n", __func__);
		return -1;
	}

	fd = open(i2c_dev, O_RDWR);
	if(fd < 0)
	{
		printf("i2c device open failed: %s\n", strerror(errno));
		return -1;
	}

	ioctl(fd, I2C_TENBIT, 0);
	ioctl(fd, I2C_SLAVE, 0x40);

	if(sht2x_softreset(fd) < 0)
	{
		printf("SHT2x softreset failure\n");
		return -2;
	}

	return fd;
}

int sht2x_get_temp_humi(int fd, float *temp, float *humi)
{
	uint8_t       buf[4];

	if(fd < 0 || !temp || !humi)
	{
		printf("%s line [%d] %s() get invalid input arguments\n", __FILE__, __LINE__, __func__);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	buf[0] = TRIGGER_TEMP_NO_HOLD;
	write(fd, buf, 1);

	msleep(85);

	memset(buf, 0, sizeof(buf));
	read(fd, buf, 3);
	//dump_buf("Temperature sample data: ", buf, 3);

	*temp = 175.72 * ((((int)buf[0] << 8) + (buf[1] & 0xfc)) / 65536.0) - 46.85;


	memset(buf, 0, sizeof(buf));
	buf[0] = TRIGGER_HUMI_NO_HOLD;
	write(fd, buf, 1);

	msleep(29);
	memset(buf, 0, sizeof(buf));

	read(fd, buf, 3);
	//dump_buf("Relative humidity sample data: ", buf, 3);

	*humi = 125 * ((((int)buf[0] << 8) + (buf[1] & 0xfc)) / 65536.0) - 6;

	return 0;
}

int sht2x_get_serialnumber(int fd, uint8_t *serialnumber, int size)
{
	uint8_t      buf[4];

	if((fd < 0) || !serialnumber || (size != 8))
	{
		printf("%s line [%d] %s() get invalid arguments\n", __FILE__, __LINE__, __func__);
		return -1;
	}

	memset(buf, 0, sizeof(buf));
	buf[0] = 0xfa;
	buf[1] = 0x0f;
	write(fd, buf, 2);

	memset(buf, 0, sizeof(buf));
	read(fd, buf, 4);

	serialnumber[5] = buf[0];
	serialnumber[4] = buf[1];
	serialnumber[3] = buf[2];
	serialnumber[2] = buf[3];

	memset(buf, 0, sizeof(buf));
	buf[0] = 0xfc;
	buf[1] = 0xc9;
	write(fd, buf, 2);

	memset(buf, 0, sizeof(buf));
	read(fd, buf, 4);

	serialnumber[1] = buf[0];
	serialnumber[0] = buf[1];
	serialnumber[7] = buf[2];
	serialnumber[6] = buf[3];

	//dump_buf("SHT2x serial number: ", serialnumber, 8);
	return 0;
}

void signal_stop(int signum)
{
	if((SIGTERM ==signum) || (SIGINT == signum))
	{
		printf("\n Close this program and stop sampling\n");
		g_stop = 0;
	}
}
