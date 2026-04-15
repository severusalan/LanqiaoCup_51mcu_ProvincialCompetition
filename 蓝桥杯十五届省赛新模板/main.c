#include <STC15F2K60S2.H>
#include "init.h"
#include "key.h"
#include "led.h"
#include "seg.h"
#include "iic.h"
#include "onewire.h"
#include "ds1302.h"
#include "ultrawave.h"
#include "math.h"

unsigned char KD,KU,KV,KO;
unsigned char KeySlow;
idata unsigned int SegSlow;

unsigned int Freq;
unsigned int Timer_1000Ms;
bit Set_Flag;

int Set_Arr[2] = {2000,-900};

idata unsigned char Seg_Buf[8]={16,16,16,16,16,16,16,16};
idata unsigned char Seg_Pos;
idata unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
idata unsigned char ucRtc[3]={0,0,0};

//定义界面显示变量
unsigned char Seg_Disp_Mode = 1;

void Key_Proc(){
    if(KeySlow<10) return;
    KeySlow=0;

    KV=Key_Read();
    KD=KV&(KO^KV);
    KU=~KV&(KO^KV);
    KO=KV;

    /* 在这里写按键逻辑 */
}

void Seg_Proc(){
    unsigned char i = 3;
    if(SegSlow<80) return;
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
   switch(Seg_Disp_Mode)
   {
    case 0:
        Seg_Buf[0] = 15;
        Seg_Buf[1] = Seg_Buf[2] = 16;
        Seg_Buf[3] = Freq / 10000 % 10;
        Seg_Buf[4] = Freq / 1000 % 10;
        Seg_Buf[5] = Freq / 100 % 10;
        Seg_Buf[6] = Freq / 10 % 10;
        Seg_Buf[7] = Freq % 10;
        while(Seg_Buf[i] == 0)
        {
            Seg_Buf[i] = 16;
            if(i++ == 7) break;
        }
    break;
    case 1:
        Seg_Buf[0] = 12;
        Seg_Buf[1] = (unsigned char)Set_Flag + 1;
        Seg_Buf[2] = Seg_Buf[3] = 16;
        Seg_Buf[4] = Set_Flag?(16 + (Set_Arr[Set_Flag] < 0)):abs(Set_Arr[Set_Flag]) / 1000 % 10;
        Seg_Buf[5] = abs(Set_Arr[Set_Flag]) / 100 % 10;
        Seg_Buf[6] = abs(Set_Arr[Set_Flag]) / 10 % 10;
        Seg_Buf[7] = abs(Set_Arr[Set_Flag]) % 10;
   }
}

void Led_Proc(){
    /* 在这里写 LED / 蜂鸣器 / 继电器逻辑 */
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
	TL0 = 0x18;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
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

    Led_Disp(ucLed);

		if(++Timer_1000Ms == 1000) 
    {
        Timer_1000Ms = 0; // 满 1 秒，清零重新计时
        // 把这 1 秒内定时器 0 数到的总脉冲数，存给 Freq
        Freq = (TH0 << 8) | TL0; 
        // 计步器清零，开始数下一秒
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
