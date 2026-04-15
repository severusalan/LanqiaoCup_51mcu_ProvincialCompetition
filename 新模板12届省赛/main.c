#include <STC15F2K60S2.H>
#include "init.h"
#include "key.h"
#include "led.h"
#include "seg.h"
#include "iic.h"
#include "onewire.h"
#include "ds1302.h"
#include "uart.h"
#include "ultrawave.h"

unsigned char KD,KU,KV,KO;
unsigned char KeySlow;
idata unsigned int SegSlow;

idata unsigned char Seg_Buf[8]={16,16,16,16,16,16,16,16};
idata unsigned char Seg_Pos;
idata unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
idata unsigned char ucRtc[3]={0,0,0};

idata unsigned char U_Rx[10];
idata unsigned char U_Flag;
idata unsigned char U_Tick;
idata unsigned char U_Index;

float Temperature; //实时温度变量
float Vol_Output; //DA输出电压值，0-5V对应0-255
idata unsigned char Temperature_Para = 25; //温度参数变量(显示)
idata unsigned char Temperature_Ctrol = 25; //温度控制变量
idata unsigned char Seg_Disp_Mode; //数码管显示模式 0-温度显示 1-参数设置 2-DA输出
bit Output_Mode; 

void Key_Proc(){
    if(KeySlow<10) return;
    KeySlow=0;

    KV=Key_Read();
    KD=KV&(KO^KV);
    KU=~KV&(KO^KV);
    KO=KV;

    /* 在这里写按键逻辑 */
    switch(KD)
    {
        case 4:
            if(++Seg_Disp_Mode==3)
            {
                Seg_Disp_Mode = 0;
            }
            if(Seg_Disp_Mode == 1) Temperature_Para = Temperature_Ctrol; //进入参数设置模式时，将当前温度控制值加载到参数显示变量中
            if(Seg_Disp_Mode == 1) Temperature_Ctrol = Temperature_Para; //退出参数设置模式时，将参数显示变量的值保存到温度控制变量中
        break;
        case 8:
            if(Seg_Disp_Mode==1)
            {
                if(--Temperature_Para == 255) //限制参数界面下限为0
                {
                    Temperature_Para = 0;
                }
            }
        break;
        case 9:
            if(Seg_Disp_Mode==1)
            {
                if(++Temperature_Para == 100) //限制参数界面上限为99
                {
                    Temperature_Para = 99;
                }
            }
        break;
        case 5:
            Output_Mode ^= 1;
        break;
    }
}

void Seg_Proc(){
    if(SegSlow<200) return;
    SegSlow=0;

    /* 在这里写数码管缓冲区赋值逻辑 */
    /*
    Seg_Buf[0]=16;
    Seg_Buf[1]=16;
    Seg_Buf[2]=16;
    Seg_Buf[3]=16;
    Seg_Buf[4]=16;
    Seg_Buf[5]=16;
    Seg_Buf[6]=16;
    Seg_Buf[7]=16;
    */
    Temperature = rd_temperature(); //实时读取温度值

    switch(Seg_Disp_Mode)
    {
        case 0:
            /* 温度显示模式 */
            Seg_Buf[0] = 12;
            Seg_Buf[1] = 16;
            Seg_Buf[2] = 16;
            Seg_Buf[3] = 16;
            Seg_Buf[4] = (unsigned char)Temperature/10%10;
            Seg_Buf[5] = (unsigned char)Temperature%10 + ',';
            Seg_Buf[6] = (unsigned int)(Temperature*100)/10%10;
            Seg_Buf[7] = (unsigned int)(Temperature*100)%10;
        break;
        case 1:
            /* 参数设置模式 */
            Seg_Buf[0] = 17;
            Seg_Buf[1] = 16;
            Seg_Buf[2] = 16;
            Seg_Buf[3] = 16;
            Seg_Buf[4] = 16;
            Seg_Buf[5] = 16;
            Seg_Buf[6] = Temperature_Para/10%10;
            Seg_Buf[7] = Temperature_Para%10;
        break;
        case 2:
            /* DA输出模式 */
            Seg_Buf[0] = 10;
            Seg_Buf[1] = 16;
            Seg_Buf[2] = 16;
            Seg_Buf[3] = 16;
            Seg_Buf[4] = 16;
            Seg_Buf[5] = Vol_Output + ',';
            Seg_Buf[6] = (unsigned int)(Vol_Output*100)/10%10;
            Seg_Buf[7] = (unsigned int)(Vol_Output*100)%10; //浮点数乘以100转整数，然后用除法和取模分离出各位数字
        break;
    }
}

void Led_Proc(){
    /* 在这里写 LED / 蜂鸣器 / 继电器逻辑 */

    unsigned char i;
    /*DAC相关*/
    if(Output_Mode == 0) //模式1
    {
        if(Temperature < Temperature_Ctrol)
        {
            Vol_Output = 0;
        }
        else
        {
            Vol_Output = 5;
        }
    }
    else //模式2
    {
        if(Temperature < 20)
        {
            Vol_Output = 1;
        }
        else if(Temperature > 40)
        {
            Vol_Output = 4;
        }
        else
        {
            Vol_Output = 0.15*(Temperature - 20) + 1;
        }
        Da_Write(Vol_Output * 51);
    }

    /*LED相关*/
    ucLed[0] = !Output_Mode;
    for(i = 0;i < 3;i ++)
    {
        ucLed[1+i] = (i == Seg_Disp_Mode);
    }
}

void Uart_Proc(){
    if(U_Index==0) return;
    if(U_Tick<10) return;

    U_Flag=0;
    U_Tick=0;

    /* 在这里写串口协议处理逻辑 */

    U_Index=0;
}

void Uart1Isr() interrupt 4
{
    if(RI==1){
        U_Flag=1;
        U_Tick=0;
        U_Rx[U_Index]=SBUF;
        RI=0;

        if(++U_Index>=10){
            U_Index=0;
        }
    }
}

void Timer1Init(void)		//1毫秒@12.000MHz
{
    AUXR &= 0xBF;
    TMOD &= 0x0F;
    TL1 = 0x18;
    TH1 = 0xFC;
    TF1 = 0;
    TR1 = 1;
    ET1 = 1;
    EA = 1;
}

void Timer1Isr() interrupt 3
{
    KeySlow++;
    SegSlow++;

    if(++Seg_Pos==8) Seg_Pos=0;

    if(Seg_Buf[Seg_Pos]>20) Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
    else Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);

    Led_Disp(ucLed);

    if(U_Flag==1){
        U_Tick++;
    }
}

//延时函数
void Delay750ms(void)	//@12.000MHz
{
	unsigned char data i, j, k;

	i = 6;
	j = 180;
	k = 26;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}


void main(){
    rd_temperature(); //预先读取一次温度值，避免第一次显示异常
    Delay750ms();
    SysInit();
    Timer1Init();
    UartInit();

    while(1){
        Key_Proc();
        Seg_Proc();
        Led_Proc();
        Uart_Proc();
    }
}
