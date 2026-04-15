/*	# 	单总线代码片段说明
	1. 	本文件夹中提供的驱动代码供参赛选手完成程序设计参考。
	2. 	参赛选手可以自行编写相关代码或以该代码为基础，根据所选单片机类型、运行速度和试题
		中对单片机时钟频率的要求，进行代码调试和修改。
*/
#include "onewire.h"
sbit DQ=P1^4;
void Delay_OneWire(unsigned int t)  
void Write_DS18B20(unsigned char dat)
unsigned char Read_DS18B20(void)
bit init_ds18b20(void)

float rd_temperature(){
	unsigned char low,high;
	init_ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0x44);
	Delay_OneWire(200);
	init_ds18b20();
	Write_DS18B20(0xcc);
	Write_DS18B20(0xbe);
	low=Read_DS18B20();
	high=Read_DS18B20();
	return (float)(high<<8|low)*0.0625;
}