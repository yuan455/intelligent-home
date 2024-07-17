# intelligent-home
基于树莓派使用sht20采集温湿度信息，以JSON格式上传至MQTT，MQTT发布开关灯指令，树莓派执行指令操作

上传温湿度数据格式：{"Temperature":"20.3", "Humidity":"50.2"}

发布指令格式：{"RedLed":"on"}, {"RedLed":"off"}，{"GreenLed":"on"}, {"GreenLed":"off"}，{"BlueLed":"on"}, {"BlueLed":"off"}

## sht20

```
make
./sht20 -i /dev/i2c-1 -h localhost -p 1883 -t test
```

-i：指定i2c总线设备

-h：指定MQTT  IP

-p：指定MQTT  端口

-t：指定MQTT 主题

-d：后台运行



## led

```
make 
./led -h localhost -p 1883 -t test
```

-h：指定MQTT  IP

-p：指定MQTT  端口

-t：指定MQTT 主题

-d：后台运行
