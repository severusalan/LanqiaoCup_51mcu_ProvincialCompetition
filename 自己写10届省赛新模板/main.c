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

unsigned char Seg_Disp_Mode; //数码管显示模式 0-频率显示界面 1-电压显示界面
unsigned int Freq; //实时频率值
unsigned int Timer_1000ms; //1000ms计时器
float Voltage; //实时电压值
unsigned char Output_Mode; //DAC输出模式 0-输出2.0V 1-输出RB2电压 
float Voltage_Output; //输出电压值
bit Seg_Flag = 1; //0-数码管功能关闭 1-数码管功能开启
bit Led_Flag = 1; //0-LED功能关闭 1-LED功能开启

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
            Seg_Disp_Mode ^= 1; //切换显示模式
        break;
        case 5:
            Output_Mode ^= 1; //切换输出模式
        break;
        case 6:
            Led_Flag ^= 1; //切换LED显示开关
        break;
        case 7:
            Seg_Flag ^= 1; //切换数码管显示开关
        break;
    }
}

void Seg_Proc(){

    if(SegSlow<200) return;
    SegSlow=0;

    Voltage = Ad_Read(0x43) / 51.0; //电压值计算公式，51.0是根据实际电路设计的分压比得出的
    if(Output_Mode == 0) Voltage_Output = 2;
    else Voltage_Output = Voltage;

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

    if(Seg_Flag == 0)
    {
        Seg_Buf[0] = 16;
        Seg_Buf[1] = 16;
        Seg_Buf[2] = 16;
        Seg_Buf[3] = 16;
        Seg_Buf[4] = 16;
        Seg_Buf[5] = 16;
        Seg_Buf[6] = 16;
        Seg_Buf[7] = 16;
    }
    else
    {
        switch(Seg_Disp_Mode)
        {
            
            case 0: //频率显示界面
                Seg_Buf[0] = 15; //F
                Seg_Buf[1] = 16;
                if(Freq >= 100000) Seg_Buf[2] = Freq / 100000 % 10;
                else Seg_Buf[2] = 16;
                if(Freq >= 10000) Seg_Buf[3] = Freq / 10000 % 10;
                else Seg_Buf[3] = 16;
                if(Freq >= 1000) Seg_Buf[4] = Freq / 1000 % 10;
                else Seg_Buf[4] = 16;
                if(Freq >= 100) Seg_Buf[5] = Freq / 100 % 10;
                else Seg_Buf[5] = 16;
                if(Freq >= 10) Seg_Buf[6] = Freq / 10 % 10;
                else Seg_Buf[6] = 16;
                Seg_Buf[7] = Freq % 10;
                break;
            case 1: //电压显示界面
                    Seg_Buf[0] = 17; //U
                    Seg_Buf[1] = 16;
                    Seg_Buf[2] = 16;
                    Seg_Buf[3] = 16;
                    Seg_Buf[4] = 16;
                    Seg_Buf[5] = (unsigned char)Voltage + ',';
                    Seg_Buf[6] = (unsigned int)(Voltage * 100) / 10 % 10;
                    Seg_Buf[7] = (unsigned int)(Voltage * 100) % 10;
                break;
        }
    }
}
    

void Led_Proc(){
    /* 在这里写 LED / 蜂鸣器 / 继电器逻辑 */
    unsigned char i;
    if(Led_Flag == 0)
    {
        ucLed[0] = 0;
        ucLed[1] = 0;
        ucLed[2] = 0;
        ucLed[3] = 0;
        ucLed[4] = 0;
        ucLed[5] = 0;
        ucLed[6] = 0;
        ucLed[7] = 0;
    }
    else
    {
        for(i = 0; i < 7; i++)
        {
            ucLed[i] = 0;
        }
        if(Seg_Disp_Mode == 0)
        {
            ucLed[0] = 0;
            ucLed[1] = 1;
        }
        else
        {
            ucLed[0] = 1;
            ucLed[1] = 0;
        }

        if(Voltage < 1.5)
        {
            ucLed[2] = 0;
        }
        else if(Voltage >= 1.5 && Voltage < 2.5)
        {
            ucLed[2] = 1;
        }
        else if(Voltage >= 2.5 && Voltage < 3.5)
        {
            ucLed[2] = 0;
        }
        else
        {
            ucLed[2] = 1;
        }

        if(Freq < 1000)
        {
            ucLed[3] = 0;
        }
        else if(Freq >= 1000 && Freq < 5000)
        {
            ucLed[3] = 1;
        }
        else if(Freq >= 5000 && Freq < 10000)
        {
            ucLed[3] = 0;
        }
        else
        {
            ucLed[3] = 1;
        }

        if(Output_Mode == 0)
        {
            ucLed[4] = 0;
        }
        else
        {
            ucLed[4] = 1;
        }
    }
    //DAC输出控制
    Da_Write(Voltage_Output * 51);
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

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
    TMOD |= 0x05;			//设置定时器模式
	TL0 = 0xFF;				//设置定时初始值
	TH0 = 0xFF;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
}

void Timer1Isr() interrupt 3
{
    KeySlow++;
    SegSlow++;

    if(++Seg_Pos==8) Seg_Pos=0;

    if(Seg_Buf[Seg_Pos]>20) Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
    else Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);

    if(Led_Flag == 1)
    {
        Led_Disp(ucLed);
    }
    else
    {
        Led_Disp((unsigned char *)"\0\0\0\0\0\0\0\0");
    }

    if(++Timer_1000ms == 1000)
    {
        Timer_1000ms = 0;
        Freq = TH0 << 8 | TL0;
        TH0 = 0;
        TL0 = 0;
    }
}

void main(){
    SysInit();
    Timer1Init();
    Timer0_Init();

    while(1){
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }
}
