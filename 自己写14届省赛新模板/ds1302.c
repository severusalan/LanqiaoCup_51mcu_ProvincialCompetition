/*	# 	DS1302代码片段说明
	1. 	本文件夹中提供的驱动代码供参赛选手完成程序设计参考。
	2. 	参赛选手可以自行编写相关代码或以该代码为基础，根据所选单片机类型、运行速度和试题
		中对单片机时钟频率的要求，进行代码调试和修改。
*/								
#include "ds1302.h"
#include <intrins.h>
sbit SCK=P1^7;
sbit SDA=P2^3;
sbit RST=P1^3;

void Write_Ds1302(unsigned  char temp) 
void Write_Ds1302_Byte( unsigned char address,unsigned char dat )     
unsigned char Read_Ds1302_Byte ( unsigned char address )

void Set_Rtc(unsigned char *ucRtc){
	unsigned char i;
	Write_Ds1302_Byte(0x8e,0x00);
	Write_Ds1302_Byte(0x80,0x80);
	for(i=0;i<3;i++){
		Write_Ds1302_Byte(0x84-2*i,ucRtc[i]/10%10<<4|ucRtc[i]%10);
	}
	Write_Ds1302_Byte(0x8e,0x80);
}

void Read_Rtc(unsigned char *ucRtc){
	unsigned char i;
	unsigned char temp;
	EA=0;
	for(i=0;i<3;i++){
		temp=Read_Ds1302_Byte(0x85-2*i);
		ucRtc[i]=temp/16*10+temp%16;
	}
	EA=1;
}