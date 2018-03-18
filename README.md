# mediot-hardware


if the wdt reset is triggert the board reboots,
when it **reboots it is checking the state of GPIO0 and GPIO15 and GPIO2**

|GPIO15|	GPIO0	|GPIO2	|Mode|
|--|--|--|--|
|0V|	0V|	3.3V	|Uart Bootloader||
|0V	|3.3V|	3.3V	|**Boot sketch**||
|3.3V	|x|	x|	SDIO mode (not used for Arduino)
**you need to make sure that the pins in the right state when a wtd happens or you call ESP.restart()
better you add some delays to prevent the wtd from triggering.**
