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

long int Freq;
unsigned int Timer_1000Ms;

float OutPut;

bit Set_Flag;
bit Echo_Flag;
bit Led_Flag;

unsigned char Timer_200Ms;

int code Step_Arr[2] = {1000,100};
int code Max_Arr[2] = {9000,900};
int code Min_Arr[2] = {1000,-900};
unsigned char Max_Time[3];
long int Freq_Max;

int Set_Arr[2] = {2000,0};

idata unsigned char Seg_Buf[8]={16,16,16,16,16,16,16,16};
idata unsigned char Seg_Pos;
idata unsigned char ucLed[8]={0,0,0,0,0,0,0,0};
idata unsigned char ucRtc[3]={13,03,05};

unsigned char Seg_Disp_Mode;

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
           if(++Seg_Disp_Mode == 4)
           {
            Echo_Flag = Set_Flag =Seg_Disp_Mode = 0;
           }
        break;
        case 5:
            if(Seg_Disp_Mode == 1)
            Set_Flag ^= 1;
            else if(Seg_Disp_Mode == 3)
            Echo_Flag ^= 1;
        break;
        case 8:
            if(Seg_Disp_Mode == 1)
            {
                Set_Arr[Set_Flag] += Step_Arr[Set_Flag];
                if(Set_Arr[Set_Flag] >= Max_Arr[Set_Flag])
                {
                    Set_Arr[Set_Flag] = Max_Arr[Set_Flag];
                }
            }
        break;
        case 9:
            if(Seg_Disp_Mode == 1)
            {
                Set_Arr[Set_Flag] -= Step_Arr[Set_Flag];
                if(Set_Arr[Set_Flag] <= Min_Arr[Set_Flag])
                {
                    Set_Arr[Set_Flag] = Min_Arr[Set_Flag];
                }
            }
        break;
    }
}

void Seg_Proc(){
    unsigned char i = 3;
    unsigned char j = 0;
    if(SegSlow<80) return;
    SegSlow=0;

    Read_Rtc(ucRtc);

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
   if(Freq_Max < Freq)
   {
        Freq_Max = Freq;
        for(j = 0;j < 3;j ++) //记录最大值时间
        {
            Max_Time[j] = ucRtc[j];
        }
   }
   switch(Seg_Disp_Mode)
   {
    case 0: //频率界面
        Seg_Buf[0] = 15;
        Seg_Buf[1] = Seg_Buf[2] = 16;
    if(Freq >= 0)
    {
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
    }
    else
    {
        for(j = 0;j < 3;j ++)
        {
            Seg_Buf[3+j] = 16;
            Seg_Buf[6] = Seg_Buf[7] = 20;
        }
    }
    break;
    case 1: //参数界面
        Seg_Buf[0] = 18;
        Seg_Buf[1] = (unsigned char)Set_Flag + 1;
        Seg_Buf[2] = Seg_Buf[3] = 16;
        Seg_Buf[4] = Set_Flag?(16 + (Set_Arr[Set_Flag] < 0)):abs(Set_Arr[Set_Flag]) / 1000 % 10;
        Seg_Buf[5] = abs(Set_Arr[Set_Flag]) / 100 % 10;
        Seg_Buf[6] = abs(Set_Arr[Set_Flag]) / 10 % 10;
        Seg_Buf[7] = abs(Set_Arr[Set_Flag]) % 10;
        if(Set_Arr[Set_Flag] == 0)
        {
            Seg_Buf[5] = Seg_Buf[6] = 16;
        }
    break;
    case 2: //时间界面
        for(j = 0;j < 3;j ++)
        {
            Seg_Buf[3*j] = ucRtc[j] / 10;
            Seg_Buf[3*j+1] = ucRtc[j] % 10;
        }
        Seg_Buf[2] = Seg_Buf[5] = 17;
    break;
    case 3:
        Seg_Buf[0] = 19;
        Seg_Buf[1] = 15 - 5*(unsigned char)Echo_Flag;
        if(Echo_Flag == 0) //回显最大频率
        {
            Seg_Buf[2] = 16;
            Seg_Buf[3] = Freq_Max / 10000 % 10;
            Seg_Buf[4] = Freq_Max / 1000 % 10;
            Seg_Buf[5] = Freq_Max / 100 % 10;
            Seg_Buf[6] = Freq_Max / 10 % 10;
            Seg_Buf[7] = Freq_Max % 10;
            while(Seg_Buf[i] == 0) //高位无值则熄灭
            {
                Seg_Buf[i] = 10;
                if(++i == 7)break;
            }
        }
        else
        {
            for(j = 0;j < 3;j ++)
            {
                Seg_Buf[2+2*j] = Max_Time[j] / 10;
                Seg_Buf[2+2*j+1] = Max_Time[j] % 10;
            }
        }
    break;
   }
}

void Led_Proc(){
    /* 在这里写 LED / 蜂鸣器 / 继电器逻辑 */

    //DA输出
    if(Freq < 0)OutPut = 0;
    else if(Freq >= 0 && Freq <= 500)OutPut = 1;
    else if(Freq >= Set_Arr[0])OutPut = 5;
    else (Freq < Set_Arr[0] && Freq > 500)OutPut = 1 + ((float)(5.0 - 1.0)/(Set_Arr[0] - 500)) * (Freq - 500);

    Da_Write(OutPut * 51.0); //DA输出0-5V对应0-255的值，乘以51是因为255/5=51

    ucLed[0] = Led_Flag & (!Seg_Disp_Mode);
    ucLed[1] = (Led_Flag & (OutPut == 5)) || (Freq < 0);
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

    if(Seg_Buf[Seg_Pos]>25) Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos]-',',1);
    else Seg_Disp(Seg_Pos,Seg_Buf[Seg_Pos],0);

    Led_Disp(ucLed);

	if(++Timer_1000Ms == 1000) 
    {
        Timer_1000Ms = 0; // 满 1 秒，清零重新计时
        // 把这 1 秒内定时器 0 数到的总脉冲数，存给 Freq
        Freq = (TH0 << 8) | TL0; 
        
        // 计步器清零，开始数下一秒
        Freq += Set_Arr[1];
        TH0 = 0;
        TL0 = 0;
    }

    if(++Timer_200Ms == 200)
    {
        Timer_200Ms = 0;
        Led_Flag ^= 1;
    }
}

void main(){
    SysInit();
    Timer1Init();
    Timer0_Init();
    Set_Rtc(ucRtc);
    while(1){
        Key_Proc();
        Seg_Proc();
        Led_Proc();
    }
}
